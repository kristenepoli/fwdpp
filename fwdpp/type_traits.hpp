#ifndef __FWDPP_TYPE_TRAITS_HPP__
#define __FWDPP_TYPE_TRAITS_HPP__

#include <type_traits>
#include <functional>
#include <fwdpp/forward_types.hpp>
#include <fwdpp/tags/diploid_tags.hpp>
#include <fwdpp/internal/type_traits.hpp>
#include <fwdpp/internal/recycling.hpp>
#include <fwdpp/internal/mutation_internal.hpp>

namespace KTfwd
{
    namespace traits
    {
        //! Evaluates to std::true_type if T inherits from KTfwd::mutation_base
        template <typename T>
        struct is_mutation
            : std::integral_constant<bool,
                                     std::is_base_of<KTfwd::mutation_base,
                                                     T>::value>
        {
        };

        //! Determine if T is/is derived from KTfwd::gamete_base
        template <typename T>
        struct is_gamete
            : std::integral_constant<bool, traits::internal::
                                               has_gamete_tag<T>::value>
        {
        };

        template <typename T>
        using is_diploid = traits::internal::is_diploid<T>;

        template <typename T>
        using is_custom_diploid = traits::internal::is_custom_diploid<T>;

        template <typename T>
        using is_diploid_t = typename is_diploid<T>::type;

        template <typename T>
        using is_custom_diploid_t = typename is_custom_diploid<T>::type;

#if __cplusplus >= 201402L
        template <typename T>
        constexpr bool is_diploid_v = is_diploid<T>::value;

        template <typename T>
        constexpr bool is_custom_diploid_v = is_custom_diploid<T>::value;
#endif
		
        //! Gives the "recycling bin" type corresponding to cont_t
        template <typename cont_t> struct recycling_bin_type
        {
            using type = KTfwd::fwdpp_internal::recycling_bin_t<
                typename cont_t::size_type>;
        };

        template <typename T>
        using recycling_bin_t = typename recycling_bin_type<T>::type;

        //! Check that a mutation model type is valid.
        template <typename mmodel_t, typename mcont_t, typename gcont_t>
        struct is_mutation_model
            : public std::
                  integral_constant<bool,
                                    std::is_same<
                                        typename std::result_of<decltype (
                                            &fwdpp_internal::
                                                mmodel_dispatcher<mmodel_t,
                                                                  typename gcont_t::
                                                                      value_type,
                                                                  mcont_t,
                                                                  recycling_bin_t<mcont_t>>)(
                                            mmodel_t &,
                                            typename gcont_t::value_type &,
                                            mcont_t &,
                                            recycling_bin_t<mcont_t> &)>::type,
                                        std::size_t>::value>
        {
        };

        template <typename recmodel_t, typename gamete_t, typename mcont_t>
        struct is_rec_model
            : std::
                  integral_constant<bool,
                                    std::is_same<
                                        typename std::result_of<recmodel_t(
                                            const gamete_t &, const gamete_t &,
                                            const mcont_t &)>::type,
                                        std::vector<double>>::value>
        {
        };

        template <typename dipvector_t, typename gcont_t, typename mcont_t>
        struct fitness_fxn_type
        {
            using type = typename std::
                conditional<std::is_base_of<KTfwd::tags::custom_diploid_t,
                                            typename dipvector_t::value_type>::
                                value,
                            std::function<double(
                                const typename dipvector_t::value_type &,
                                const gcont_t &, const mcont_t &)>,
                            std::function<double(
                                const typename gcont_t::value_type &,
                                const typename gcont_t::value_type &,
                                const mcont_t &)>>::type;
        };

        template <typename dipvector_t, typename gcont_t, typename mcont_t>
        using fitness_fxn_t =
            typename fitness_fxn_type<dipvector_t, gcont_t, mcont_t>::type;

        //! Gives the recombination model function signature corresponding to
        //! gcont_t,mcont_t
        template <typename gcont_t, typename mcont_t>
        using recmodel_t = std::function<std::vector<double>(
            const typename gcont_t::value_type &,
            const typename gcont_t::value_type &, const mcont_t &)>;

        /*!
          Gives the mutation model function signature corresponding to mcont_t.

          Applies to mutation policies that only take recycling bins and
          mcont_t *
          as arguments
        */
        template <typename mcont_t>
        using mmodel_t = std::function<std::size_t(recycling_bin_t<mcont_t> &,
                                                   mcont_t &)>;

        /*!
          Gives mutation model function signature for models requiring gametes
          as arguments
        */
        template <typename mcont_t, typename gcont_t>
        using mmodel_gamete_t
            = std::function<std::size_t(recycling_bin_t<mcont_t> &,
                                        typename gcont_t::value_type &,
                                        mcont_t &)>;
    }
}
#endif
