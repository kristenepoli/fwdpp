/*! \include wfmlts.cc
 * Multi-locus/region Wright-Fisher simulation with tree sequences.
 *
 * See the following paper for background and motivation:
 * Kelleher, Jerome, Kevin Thornton, Jaime Ashander, and Peter Ralph. 2018.
 * “Efficient Pedigree Recording for Fast Population Genetics Simulation.”
 * bioRxiv. https://doi.org/10.1101/248500.
 *
 */

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <fstream>
#include <string>
#include <fwdpp/ts/table_collection.hpp>
#include <fwdpp/ts/table_simplifier.hpp>
#include <fwdpp/ts/recycling.hpp>
#include <fwdpp/ts/remove_fixations_from_gametes.hpp>
#include <fwdpp/interlocus_recombination.hpp>
#include <fwdpp/util.hpp>
#include <fwdpp/fitness_models.hpp>
#include <fwdpp/extensions/callbacks.hpp>
#include <fwdpp/poisson_xover.hpp>
#include <fwdpp/recbinder.hpp>
#include <fwdpp/simparams.hpp>

#include "simplify_tables.hpp"
#include "evolve_generation_ts.hpp"
#include "calculate_fitnesses.hpp"
#include "tree_sequence_examples_common.hpp"

namespace po = boost::program_options;
using poptype = multi_locus_poptype;
using GSLrng = fwdpp::GSLrng_mt;

struct multilocus_multiplicative
{
    //NOTE: older compilers may not accept initialization
    //of this type into fwdpp::simparams unless this default
    //constructor is defined:
    multilocus_multiplicative() {}
    inline double
    operator()(const poptype::diploid_t &diploid,
               const poptype::gcont_t &gametes,
               const poptype::mcont_t &mutations) const
    {
        double rv = 1.;
        for (auto &&genotype : diploid)
            {
                rv *= fwdpp::multiplicative_diploid(fwdpp::fitness(2.))(
                    genotype, gametes, mutations);
            }
        return std::max(0., rv);
    }
};

int
main(int argc, char **argv)
{
    options o;

    auto main_options = generate_main_options(o);
    main_options.add_options()(
        "nloci", po::value<int>(&o.nloci),
        "Number of loci.  Free recombination between them.");

    auto dfe_options = generate_dfe_options(o);
    auto testing_options = generate_testing_options(o);
    main_options.add(dfe_options);
    main_options.add(testing_options);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, main_options), vm);
    po::notify(vm);

    if (vm.count("help") || argc == 1)
        {
            std::cout << main_options << '\n';
            std::exit(1);
        }

    validate_primary_options(o);

    GSLrng rng(o.seed);

    std::vector<std::pair<double, double>> locus_boundaries;
    for (int i = 0; i < o.nloci; ++i)
        {
            locus_boundaries.emplace_back(i, i + 1);
        }
    poptype pop(o.N, locus_boundaries);

    //NOTE: genome length must correspond to make position, as specified
    //in locus_boundaries!!!
    fwdpp::ts::table_collection tables(2 * pop.diploids.size(), 0, 0, o.nloci);
    fwdpp::ts::table_simplifier simplifier(o.nloci);
    unsigned generation = 1;
    double recrate = o.rho / static_cast<double>(4 * o.N);

    auto get_selection_coefficient
        = make_dfe(o.N, rng, o.mean, o.shape, o.scoeff);
    const auto generate_h = [&o]() { return o.dominance; };
    std::vector<std::function<std::vector<double>(void)>>
        intralocus_recombination;

    std::vector<std::function<std::vector<fwdpp::uint_t>(
        std::queue<std::size_t> &, poptype::mcont_t &)>>
        mmodels;
    for (int i = 0; i < o.nloci; ++i)
        {
            intralocus_recombination.emplace_back(fwdpp::recbinder(
                fwdpp::poisson_xover(recrate, i, i + 1), rng.get()));
            const auto generate_mutation_position
                = [&rng, i]() { return gsl_ran_flat(rng.get(), i, i + 1); };
            const auto make_mutation = [&pop, &rng, &generation,
                                        generate_mutation_position,
                                        get_selection_coefficient, generate_h](
                                           std::queue<std::size_t> &recbin,
                                           poptype::mcont_t &mutations) {
                return fwdpp::infsites_popgenmut(
                    recbin, mutations, rng.get(), pop.mut_lookup, generation,
                    // 1.0 signifies 100% of mutations will be selected
                    1.0, generate_mutation_position, get_selection_coefficient,
                    generate_h);
            };
            const auto mmodel
                = [&rng, &o, make_mutation](std::queue<std::size_t> &recbin,
                                            poptype::mcont_t &mutations) {
                      std::vector<fwdpp::uint_t> rv;
                      unsigned nmuts = gsl_ran_poisson(rng.get(), o.mu);
                      for (unsigned m = 0; m < nmuts; ++m)
                          {
                              rv.push_back(make_mutation(recbin, mutations));
                          }
                      std::sort(begin(rv), end(rv),
                                [&mutations](const fwdpp::uint_t a,
                                             const fwdpp::uint_t b) {
                                    return mutations[a].pos < mutations[b].pos;
                                });
                      return rv;
                  };
            mmodels.emplace_back(std::move(mmodel));
        }

    // Evolve pop for 20N generations
    fwdpp::ts::TS_NODE_INT first_parental_index = 0,
                           next_index = 2 * pop.diploids.size();
    bool simplified = false;
    std::vector<std::size_t> individual_labels(o.N);
    std::iota(individual_labels.begin(), individual_labels.end(), 0);
    std::vector<std::size_t> individuals;
    if (o.ancient_sample_size > 0)
        {
            individuals.resize(o.ancient_sample_size);
        }
    std::vector<double> fitnesses;
    std::vector<diploid_metadata> ancient_sample_metadata;
    const auto update_offspring = [](std::size_t, std::size_t, std::size_t) {};

    // GOTCHA: we calculate fitnesses and generate our lookup table
    // here, based on our initial/monomorphic population.
    // The reason that we do this is that we will then update
    // these data AFTER each call to the evolution function,
    // so that fitnesses == those of current pop.diploids,
    // meaning that we can record a correct fitness
    // value as ancient sample metadata.  This is a logic
    // issue that is easy to goof.
    auto ff = multilocus_multiplicative();

    std::vector<double> between_locus_recombination_rate(o.nloci - 1, 0.5);
    auto interlocus_rec = fwdpp::make_binomial_interlocus_rec(
        rng.get(), between_locus_recombination_rate.data(),
        between_locus_recombination_rate.size());

    auto genetics = fwdpp::make_genetic_parameters(
        std::move(ff), std::move(mmodels), std::move(intralocus_recombination),
        std::move(interlocus_rec));
    auto lookup = calculate_fitnesses(pop, fitnesses, genetics.gvalue);
    for (; generation <= 10 * o.N; ++generation)
        {
            auto pick1 = [&lookup, &rng]() {
                return gsl_ran_discrete(rng.get(), lookup.get());
            };
            auto pick2 = [&lookup, &rng](const std::size_t /*p1*/) {
                return gsl_ran_discrete(rng.get(), lookup.get());
            };
            evolve_generation(rng, pop, genetics, o.N, pick1, pick2,
                              update_offspring, generation, tables,
                              first_parental_index, next_index);
            // Recalculate fitnesses and the lookup table.
            lookup = calculate_fitnesses(pop, fitnesses, genetics.gvalue);
            if (generation % o.gcint == 0.0)
                {
                    auto rv = simplify_tables(
                        pop, generation, pop.mcounts_from_preserved_nodes,
                        tables, simplifier, tables.num_nodes() - 2 * o.N,
                        2 * o.N, o.preserve_fixations);
                    if (!o.preserve_fixations)
                        {
                            genetics.mutation_recycling_bin
                                = fwdpp::ts::make_mut_queue(
                                    pop.mcounts,
                                    pop.mcounts_from_preserved_nodes);
                        }
                    else
                        {
                            genetics.mutation_recycling_bin
                                = fwdpp::ts::make_mut_queue(
                                    rv.second, pop.mutations.size());
                        }
                    simplified = true;
                    next_index = tables.num_nodes();
                    first_parental_index = 0;

                    // When tracking ancient samples, the node ids of those samples change.
                    // Thus, we need to remap our metadata upon simplification
                    for (auto &md : ancient_sample_metadata)
                        {
                            md.n1 = rv.first[md.n1];
                            md.n2 = rv.first[md.n2];
                            assert(md.n1 != fwdpp::ts::TS_NULL_NODE);
                            assert(md.n2 != fwdpp::ts::TS_NULL_NODE);
                        }
                }
            else
                {
                    simplified = false;
                    first_parental_index = next_index;
                    next_index += 2 * o.N;

                    // TODO: update or remove this bit
                    // The following (commented-out) block
                    // shows that it is possible to mix mutation
                    // counting strategies in the right situations.
                    // However, the process_gametes call is a quadratic
                    // operation and the overall effect on run time
                    // and peak RAM usage is modest at best in the big
                    // picture...

                    //if (tables.preserved_nodes.empty())
                    //    {
                    //        fwdpp::fwdpp_internal::process_gametes(
                    //            pop.gametes, pop.mutations, pop.mcounts);
                    //        fwdpp::fwdpp_internal::gamete_cleaner(
                    //            pop.gametes, pop.mutations, pop.mcounts, 2 * N,
                    //            std::true_type());
                    //        fwdpp::update_mutations(
                    //            pop.mutations, pop.fixations,
                    //            pop.fixation_times, pop.mut_lookup,
                    //            pop.mcounts, generation, 2 * N);
                    //        mutation_recycling_bin
                    //            = fwdpp::fwdpp_internal::make_mut_queue(
                    //                pop.mcounts);
                    //        auto itr = std::remove_if(
                    //            tables.mutation_table.begin(),
                    //            tables.mutation_table.end(),
                    //            [&pop](const fwdpp::ts::mutation_record &mr) {
                    //                return pop.mcounts[mr.key] == 0;
                    //            });
                    //        tables.mutation_table.erase(
                    //            itr, tables.mutation_table.end());
                    //    }
                }
            if (o.ancient_sampling_interval > 0
                && generation % o.ancient_sampling_interval == 0.0
                // This last check forbids us recording the
                // final generation as ancient samples.
                && generation < 10 * o.N)
                {
                    gsl_ran_choose(
                        rng.get(), individuals.data(), individuals.size(),
                        individual_labels.data(), individual_labels.size(),
                        sizeof(std::size_t));
                    for (auto i : individuals)
                        {
                            auto x = fwdpp::ts::get_parent_ids(
                                first_parental_index, i, 0);
                            assert(x.first >= first_parental_index);
                            assert(x.second >= first_parental_index);
                            assert(x.first < tables.num_nodes());
                            assert(x.second < tables.num_nodes());
                            assert(tables.node_table[x.first].time
                                   == generation);
                            assert(tables.node_table[x.second].time
                                   == generation);
                            assert(std::find(tables.preserved_nodes.begin(),
                                             tables.preserved_nodes.end(),
                                             x.first)
                                   == tables.preserved_nodes.end());
                            assert(std::find(tables.preserved_nodes.begin(),
                                             tables.preserved_nodes.end(),
                                             x.second)
                                   == tables.preserved_nodes.end());
                            tables.preserved_nodes.push_back(x.first);
                            tables.preserved_nodes.push_back(x.second);
                            // Record the metadata for our ancient samples
                            // Here, we record each individual's actual fitness.
                            // If we wanted relative fitness, then
                            // we'd have to copy fitnesses into a temp
                            // and normalize it appropriately.
                            ancient_sample_metadata.emplace_back(
                                i, generation, fitnesses[i], x.first,
                                x.second);
                        }
                }
        }
    if (!simplified)
        {
            auto rv = simplify_tables(pop, generation,
                                      pop.mcounts_from_preserved_nodes, tables,
                                      simplifier, tables.num_nodes() - 2 * o.N,
                                      2 * o.N, o.preserve_fixations);
            if (o.preserve_fixations)
                {
                    std::vector<std::int32_t> samples(2 * o.N);
                    std::iota(samples.begin(), samples.end(), 0);
                    fwdpp::ts::count_mutations(
                        tables, pop.mutations, samples, pop.mcounts,
                        pop.mcounts_from_preserved_nodes);
                }
            confirm_mutation_counts(pop, tables);
            // When tracking ancient samples, the node ids of those samples change.
            // Thus, we need to remap our metadata upon simplification
            for (auto &md : ancient_sample_metadata)
                {
                    md.n1 = rv.first[md.n1];
                    md.n2 = rv.first[md.n2];
                    assert(md.n1 != fwdpp::ts::TS_NULL_NODE);
                    assert(md.n2 != fwdpp::ts::TS_NULL_NODE);
                }
        }
    // If we have done things correctly, then our
    // ancient sample metadata must match up with
    // what is in our node table.
    for (auto &mr : ancient_sample_metadata)
        {
            if (tables.node_table[mr.n1].time != mr.time
                || tables.node_table[mr.n2].time != mr.time)
                {
                    throw std::runtime_error(
                        "invalid ancient sample metadata");
                }
        }
    assert(tables.input_left.size() == tables.edge_table.size());
    assert(tables.output_right.size() == tables.edge_table.size());
    std::vector<fwdpp::ts::TS_NODE_INT> s(2 * o.N);
    std::iota(s.begin(), s.end(), 0);

    auto neutral_muts = apply_neutral_mutations(
        o, rng, tables, pop, genetics.mutation_recycling_bin);

    fwdpp::ts::count_mutations(tables, pop.mutations, s, pop.mcounts,
                               pop.mcounts_from_preserved_nodes);
    for (std::size_t i = 0; i < pop.mutations.size(); ++i)
        {
            if (pop.mutations[i].neutral)
                {
                    if (!pop.mcounts[i]
                        && !pop.mcounts_from_preserved_nodes[i])
                        {
                            throw std::runtime_error(
                                "invalid final mutation count");
                        }
                }
        }
    std::cout << neutral_muts << '\n';

    execute_expensive_leaf_test(o, tables, s);
    execute_matrix_test(o, pop, tables, s);
    execute_serialization_test(o, tables);
    write_sfs(o, rng, tables, s, pop.mutations);
}

