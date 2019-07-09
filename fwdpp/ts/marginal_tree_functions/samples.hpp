#ifndef FWDPP_TS_MARGINAL_TREE_FUNCTIONS_SAMPLES_HPP__
#define FWDPP_TS_MARGINAL_TREE_FUNCTIONS_SAMPLES_HPP__

#include <stdexcept>
#include "../marginal_tree.hpp"

namespace fwdpp
{
    namespace ts
    {
        class samples_iterator
        /// \brief Faciliate traversal of the samples descending from a node
        /// \headerfile fwdpp/ts/marginal_tree_functions/samples.hpp
        {
          private:
            const marginal_tree &t;
            TS_NODE_INT current_sample, right_sample;

            inline const marginal_tree &
            init_marginal(const marginal_tree &m)
            {
                if (!m.advancing_sample_list())
                    {
                        throw std::invalid_argument(
                            "samples lists are not being updated");
                    }
                return m;
            }

            inline TS_NODE_INT
            init_left_sample(TS_NODE_INT u)
            {
                if (static_cast<std::size_t>(u) >= t.size())
                    {
                        throw std::invalid_argument("node index out of range");
                    }
                return t.left_sample[u];
            }

            inline TS_NODE_INT
            init_right_sample(TS_NODE_INT u)
            {
                if (static_cast<std::size_t>(u) >= t.size())
                    {
                        throw std::invalid_argument("node index out of range");
                    }
                return t.right_sample[u];
            }

          public:
            samples_iterator(const marginal_tree &m, TS_NODE_INT u)
                : t(init_marginal(m)), current_sample{ init_left_sample(u) },
                  right_sample(init_right_sample(u))
            /// \param m A marginal_tree
            /// \param u A node index
            {
            }

            inline TS_NODE_INT
            operator()()
            /// Advance to the next sample
            ///
            /// Returns fwdpp::ts::TS_NULL_NODE when no more samples remain.
            {
                if (current_sample == TS_NULL_NODE)
                    {
                        //end of iteration
                        return current_sample;
                    }
                auto c = current_sample;
                if (c == right_sample)
                    {
                        // We are at the end of the samples list for this node,
                        // so we ensure that iteration will end
                        current_sample = TS_NULL_NODE;
                    }
                else
                    {
                        current_sample = t.next_sample[current_sample];
                    }
                return c;
            }

            template <typename F>
            inline bool
            operator()(const F &f)
            /// Apply a function to each sample
            /// \param f A function equivalent to void (*process_sample)(TS_NODE_INT)
            ///
            /// Returns false to signify end of iteration.
            {
                auto s = this->operator()();
                bool rv = (s != TS_NULL_NODE);
                if (rv)
                    {
                        f(s);
                    }
                return rv;
            }
        };

        template <typename F>
        inline void
        process_samples(const marginal_tree &m, TS_NODE_INT u, const F &f)
        /// \brief Apply a function to samples of node \a u
        /// \param m A fwdpp::ts::marginal_tree
        /// \param u Index a fwdpp::ts::node
        /// \param f A function equivalent to void (*process_samples)(TS_NODE_INT)
        {
            samples_iterator si(m, u);
            while (si(f))
                {
                }
        }

        inline std::vector<TS_NODE_INT>
        get_samples(const marginal_tree &m, TS_NODE_INT u)
        /// Return a vector of samples descending from node \a u in marginal_tree \a m.
        {
            std::vector<TS_NODE_INT> rv;
            process_samples(m, u, [&rv](TS_NODE_INT x) { rv.push_back(x); });
            return rv;
        }

        inline int
        num_samples(const marginal_tree &m, TS_NODE_INT u)
        /// Return the number of samples descending from node \a u in marginal_tree \a m.
        {
            int nsamples = 0;
            process_samples(m, u,
                            [&nsamples](TS_NODE_INT /*x*/) { ++nsamples; });
            return nsamples;
        }
    } // namespace ts
} // namespace fwdpp

#endif
