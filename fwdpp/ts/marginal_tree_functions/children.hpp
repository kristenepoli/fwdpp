#ifndef FWDPP_TS_MARGINAL_TREE_FUNCTIONS_CHILDREN_HPP
#define FWDPP_TS_MARGINAL_TREE_FUNCTIONS_CHILDREN_HPP

#include "../marginal_tree.hpp"

namespace fwdpp
{
    namespace ts
    {
        class child_iterator
        {
          private:
            bool left_to_right;
            TS_NODE_INT current_child;
            std::vector<TS_NODE_INT>::const_iterator child_begin, child_end,
                sib_begin, sib_end;

            inline TS_NODE_INT
            init_current_child(const marginal_tree& m, TS_NODE_INT n)
            {
                if (n >= m.left_child.size())
                    {
                        throw std::invalid_argument("node id is out of range");
                    }
                if (left_to_right)
                    {
                        return m.left_child[n];
                    }
                return m.right_child[n];
            }

          public:
            child_iterator(const marginal_tree& m, TS_NODE_INT n,
                           bool left_to_right_traversal)
                : left_to_right(left_to_right_traversal),
                  current_child(init_current_child(m, n)),
                  child_begin(left_to_right ? begin(m.left_child)
                                            : begin(m.right_child)),
                  child_end(left_to_right ? end(m.left_child)
                                          : end(m.right_child)),
                  sib_begin(left_to_right ? begin(m.left_sib)
                                          : begin(m.right_sib)),
                  sib_end(left_to_right ? end(m.left_sib) : end(m.right_sib))
            {
            }

            inline TS_NODE_INT
            operator()()
            {
                auto c = current_child;
                if (sib_begin + current_child >= child_end)
                    {
                        throw std::runtime_error("child iteration error");
                    }
                current_child = *(sib_begin + current_child);
                return c;
            }
        };

        template <typename F>
        inline void
        process_children(const marginal_tree& m, TS_NODE_INT n,
                         bool left_to_right_traversal, const F& f)
        {
            child_iterator ci(m, n, left_to_right_traversal);
            auto c = ci();
            while (c != TS_NULL_NODE)
                {
                    f(c);
                    c = ci();
                }
        }

        inline std::vector<TS_NODE_INT>
        get_children(const marginal_tree& m, TS_NODE_INT n,
                     bool left_to_right_traversal)
        {
            std::vector<TS_NODE_INT> rv;
            process_children(m, n, left_to_right_traversal,
                             [&rv](const TS_NODE_INT c) { rv.push_back(c); });
            return rv;
        }

        inline int
        num_children(const marginal_tree& m, TS_NODE_INT n)
        {
            int nc = 0;
            process_children(m, n, true,
                             [&nc](const TS_NODE_INT /*c*/) { ++nc; });
            return nc;
        }
    } // namespace ts
} // namespace fwdpp

#endif
