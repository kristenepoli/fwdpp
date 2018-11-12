#ifndef FWDPP_EXAMPLES_SIMPLIFY_TABLES_HPP
#define FWDPP_EXAMPLES_SIMPLIFY_TABLES_HPP

#include <cstdint>
#include <vector>
#include <fwdpp/ts/table_collection.hpp>
#include <fwdpp/ts/table_simplifier.hpp>
#include <fwdpp/ts/count_mutations.hpp>
#include <fwdpp/ts/remove_fixations_from_gametes.hpp>
#include <fwdpp/ts/recycling.hpp>
#include "confirm_mutation_counts.hpp"

template <typename poptype>
std::vector<fwdpp::ts::TS_NODE_INT>
simplify_tables(poptype &pop, const fwdpp::uint_t generation,
                std::vector<fwdpp::uint_t> &mcounts_from_preserved_nodes,
                fwdpp::ts::table_collection &tables,
                fwdpp::ts::table_simplifier &simplifier,
                const fwdpp::ts::TS_NODE_INT first_sample_node,
                const std::size_t num_samples)
{
    tables.sort_tables(pop.mutations);
    std::vector<std::int32_t> samples(num_samples);
    std::iota(samples.begin(), samples.end(), first_sample_node);
    auto idmap = simplifier.simplify(tables, samples, pop.mutations);
    tables.build_indexes();
    for (auto &s : samples)
        {
            s = idmap[s];
        }
#ifndef NDEBUG
    for (auto &s : tables.preserved_nodes)
        {
            assert(idmap[s] != 1);
        }
#endif
    fwdpp::ts::count_mutations(tables, pop.mutations, samples, pop.mcounts,
                               mcounts_from_preserved_nodes);
    tables.mutation_table.erase(
        std::remove_if(
            tables.mutation_table.begin(), tables.mutation_table.end(),
            [&pop, &mcounts_from_preserved_nodes](
                const fwdpp::ts::mutation_record &mr) {
                return pop.mcounts[mr.key] == 2 * pop.diploids.size()
                       && mcounts_from_preserved_nodes[mr.key] == 0;
            }),
        tables.mutation_table.end());
    fwdpp::ts::remove_fixations_from_gametes(
        pop.gametes, pop.mutations, pop.mcounts, mcounts_from_preserved_nodes,
        2 * pop.diploids.size(), false);

    fwdpp::ts::flag_mutations_for_recycling(
        pop, mcounts_from_preserved_nodes, 2 * pop.diploids.size(), generation,
        std::false_type(), std::false_type());
    confirm_mutation_counts(pop, tables);
    return idmap;
}
#endif
