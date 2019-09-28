#ifndef TS_EXAMPLES_TYPES_HPP
#define TS_EXAMPLES_TYPES_HPP

#include <fwdpp/diploid_population.hpp>
#include <fwdpp/popgenmut.hpp>

using single_locus_poptype = fwdpp::diploid_population<fwdpp::popgenmut>;

struct diploid_metadata
{
    std::size_t individual;
    double time, fitness;
    fwdpp::ts::TS_NODE_INT n1, n2;
    diploid_metadata(std::size_t i, double t, double w,
                     fwdpp::ts::TS_NODE_INT a, fwdpp::ts::TS_NODE_INT b)
        : individual(i), time(t), fitness(w), n1(a), n2(b)
    {
    }
};

#endif
