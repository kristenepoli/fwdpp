#ifndef FWDPP_TS_RECORDING_DIPLOID_OFFSPRING_HPP
#define FWDPP_TS_RECORDING_DIPLOID_OFFSPRING_HPP

#include <tuple>
#include <limits>
#include <vector>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <fwdpp/ts/definitions.hpp>

namespace fwdpp
{
    namespace ts
    {
        template <typename TableCollectionType>
        inline void
        split_breakpoints_add_edges(const std::vector<double>& breakpoints,
                                    const std::tuple<TS_NODE_INT, TS_NODE_INT>& parents,
                                    const TS_NODE_INT next_index,
                                    TableCollectionType& tables)
        {
            if (breakpoints.front() != 0.0)
                {
                    tables.push_back_edge(0., breakpoints.front(), std::get<0>(parents),
                                          next_index);
                }
            // TODO: replace with exception via a debug mode
            assert(std::count(begin(breakpoints), end(breakpoints),
                              std::numeric_limits<double>::max())
                   == 1);
            assert(breakpoints.back() == std::numeric_limits<double>::max());
            for (unsigned j = 1; j < breakpoints.size(); ++j)
                {
                    double a = breakpoints[j - 1];
                    double b = (j < breakpoints.size() - 1) ? breakpoints[j]
                                                            : tables.genome_length();
                    if (b <= a)
                        {
                            throw std::invalid_argument("right must be > left");
                        }
                    if (j % 2 == 0.)
                        {
                            tables.push_back_edge(a, b, std::get<0>(parents),
                                                  next_index);
                        }
                    else
                        {
                            tables.push_back_edge(a, b, std::get<1>(parents),
                                                  next_index);
                        }
                }
        }

        template <typename TableCollectionType>
        inline void
        split_breakpoints(const std::vector<double>& breakpoints,
                          const std::tuple<TS_NODE_INT, TS_NODE_INT>& parents,
                          const TS_NODE_INT next_index, TableCollectionType& tables)
        {
            if (breakpoints.empty())
                {
                    tables.push_back_edge(0., tables.genome_length(),
                                          std::get<0>(parents), next_index);
                    return;
                }
            auto itr
                = std::adjacent_find(std::begin(breakpoints), std::end(breakpoints));
            if (itr == std::end(breakpoints))
                {
                    split_breakpoints_add_edges(breakpoints, parents, next_index, tables);
                }
            else
                {
                    // Here, we need to reduce the input
                    // breakpoints to only those seen
                    // an odd number of times.
                    // Even numbers of the same breakpoint
                    // are "double x-overs" and thus
                    // cannot affect the genealogy.
                    std::vector<double> odd_breakpoints;
                    auto start = breakpoints.begin();
                    while (itr < breakpoints.end())
                        {
                            auto not_equal = std::find_if(
                                itr, breakpoints.end(),
                                [itr](const double d) { return d != *itr; });
                            int even = (std::distance(itr, not_equal) % 2 == 0.0);
                            odd_breakpoints.insert(odd_breakpoints.end(), start,
                                                   itr + 1 - even);
                            start = not_equal;
                            itr = std::adjacent_find(start, std::end(breakpoints));
                        }
                    odd_breakpoints.insert(odd_breakpoints.end(), start,
                                           breakpoints.end());
                    split_breakpoints_add_edges(odd_breakpoints, parents, next_index, tables);
                }
        }

        template <typename TableCollectionType>
        inline std::size_t
        register_diploid_offspring(const std::vector<double>& breakpoints,
                                   const std::tuple<TS_NODE_INT, TS_NODE_INT>& parents,
                                   const std::int32_t population, const double time,
                                   TableCollectionType& tables)
        {
            // TODO: carefully document how to index node times.
            auto next_index = tables.emplace_back_node(population, time);
            if (next_index >= std::numeric_limits<TS_NODE_INT>::max())
                {
                    throw std::invalid_argument("node index too large");
                }
            split_breakpoints(breakpoints, parents, next_index, tables);
            return next_index;
        }
    }
}

#endif
