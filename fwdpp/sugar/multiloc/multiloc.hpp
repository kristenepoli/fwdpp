#ifndef __FWDPP_SUGAR_MULTILOC_MULTILOC_HPP__
#define __FWDPP_SUGAR_MULTILOC_MULTILOC_HPP__

#include <type_traits>
#include <vector>
#include <fwdpp/forward_types.hpp>
#include <fwdpp/sugar/serialization.hpp>
#include <fwdpp/sugar/poptypes/tags.hpp>

namespace KTfwd {
  namespace sugar {
    /*!
      \brief Abstraction of what is needed to simulate a multilocus
      simulation using an individual-based sampler from fwdpp
      
      All that is missing is the mutation_type and the container types.
      
      Does not allow copy construction/assignment!
    */
    template<typename mutation_type,
	     typename mlist,
	     typename glist,
	     typename dipvector,
	     typename vglist,
	     typename mvector,
	     typename ftvector,
	     typename lookup_table_type>
    struct multiloc
    {
      //Dispatch tags for other parts of sugar layer
      using popmodel_t = sugar::MULTILOC_TAG;
      
      //Typedefs for various container
      using mutation_t = mutation_type;
      using gamete_t = typename glist::value_type;
      using dipvector_t = dipvector;
      using diploid_t = typename dipvector_t::value_type;
      using mlist_t = mlist;
      using glist_t = glist;
      using vglist_t = vglist;
      using lookup_table_t = lookup_table_type;
    };
  }
}

#endif