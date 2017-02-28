#ifndef FWDPP_INTERNAL_TYPE_TRAITS_HPP
#define FWDPP_INTERNAL_TYPE_TRAITS_HPP
#include <type_traits>
#include <fwdpp/internal/void_t.hpp>
#include <fwdpp/internal/mutation_internal.hpp>
#include <utility>

namespace KTfwd
{
    namespace traits
    {
        namespace internal
        {
            template <typename T, typename = void>
            struct is_diploid : std::false_type
            {
            };

            template <typename T>
            struct is_diploid<T, typename traits::internal::void_t<
                                     typename T::first_type,
                                     typename T::second_type>::type>
                : std::integral_constant<bool,
                                         std::is_integral<
                                             typename T::first_type>::value
                                             && std::is_same<
                                                    typename T::first_type,
                                                    typename T::second_type>::
                                                    value>
            {
            };

            template <typename T, typename = void>
            struct is_multilocus_diploid : std::false_type
            {
            };

            template <typename T>
            struct is_multilocus_diploid<T, typename void_t<
                                                typename T::value_type>::type>
                : std::integral_constant<bool,
                                         is_diploid<
                                             typename T::value_type>::value>
            {
            };

            template <typename T, typename = void>
            struct is_custom_diploid : std::false_type
            {
            };

            template <typename T>
            struct is_custom_diploid<T, typename void_t<
                                            typename T::first_type,
                                            typename T::second_type>::type>
                : std::
                      integral_constant<bool,
                                        is_diploid<T>::value
                                            && !std::
                                                   is_same<std::pair<
                                                               typename T::
                                                                   first_type,
                                                               typename T::
                                                                   second_type>,
                                                           T>::value>
            {
            };

            template <typename dipvector_t, typename gcont_t, typename mcont_t,
                      typename = void>
            struct fitness_fxn
            {
                using type = void;
            };

            template <typename dipvector_t, typename gcont_t, typename mcont_t>
            struct fitness_fxn<dipvector_t, gcont_t, mcont_t,
                               typename void_t<
                                   typename dipvector_t::value_type,
                                   typename gcont_t::value_type,
                                   typename mcont_t::value_type>::type>
            {
                using type = typename std::
                    conditional<(is_diploid<
                                     typename dipvector_t::value_type>::value
                                 || is_multilocus_diploid<
                                        typename dipvector_t::value_type>::
                                        value)
                                    && is_gamete<
                                           typename gcont_t::value_type>::value
                                    && is_mutation<typename mcont_t::
                                                       value_type>::value,
                                std::function<double(
                                    const typename dipvector_t::value_type &,
                                    const gcont_t &, const mcont_t &)>,
                                void>::type;
            };

            template <
                typename ff, typename dipvector_t, typename gcont_t,
                typename mcont_t,
                typename ffxn_t =
                    typename fitness_fxn<dipvector_t, gcont_t, mcont_t>::type>
            struct is_fitness_fxn
                : std::integral_constant<bool,
                                         !std::is_void<ffxn_t>::value
                                             && std::is_convertible<ff,
                                                                    ffxn_t>::
                                                    value>
            {
            };

            template <typename mmodel_t, typename mcont_t, typename gcont_t,
                      typename = void>
            struct is_mutation_model : std::false_type
            {
            };

            template <typename gamete_t, typename mutation_t>
            struct check_mmodel_types
                : std::integral_constant<bool,
                                         is_gamete<gamete_t>::value
                                             && is_mutation<mutation_t>::value>
            {
            };

            template <typename mmodel_t, typename gcont_t, typename mcont_t,
                      typename gamete_t = typename gcont_t::value_type,
                      typename mutation_t = typename mcont_t::value_type,
                      typename recbin = recycling_bin_t<mcont_t>>
            struct dispatchable_mmodel
                : std::is_same<
                      typename std::result_of<decltype (
                          &fwdpp_internal::mmodel_dispatcher<mmodel_t,
                                                             gamete_t, mcont_t,
                                                             recbin>)(
                          mmodel_t &, gamete_t &, mcont_t &, recbin &)>::type,
                      std::size_t>
            {
            };

            template <typename mmodel_t, typename mcont_t, typename gcont_t>
            struct is_mutation_model<mmodel_t, mcont_t, gcont_t,
                                     typename void_t<
                                         typename mcont_t::value_type,
                                         typename gcont_t::value_type>::type>
                : std::integral_constant<bool,
                                         check_mmodel_types<
                                             typename gcont_t::value_type,
                                             typename mcont_t::value_type>::
                                                 value
                                             && dispatchable_mmodel<mmodel_t,
                                                                    gcont_t,
                                                                    mcont_t>::
                                                    value>
            {
            };

            template <typename recmodel_t, typename gamete_t, typename mcont_t,
                      typename = void>
            struct is_rec_model : std::false_type
            {
            };

            template <typename recmodel_t, typename gamete_t, typename mcont_t,
                      typename result_t = typename std::result_of<recmodel_t(
                          const gamete_t &, const gamete_t &,
                          const mcont_t &)>::type>
            using is_rec_model_SFINAE_base = std::
                integral_constant<bool,
                                  check_mmodel_types<gamete_t,
                                                     typename mcont_t::
                                                         value_type>::value
                                      && std::is_same<result_t,
                                                      std::vector<double>>::
                                             value>;

            template <typename recmodel_t, typename gamete_t, typename mcont_t>
            struct is_rec_model<recmodel_t, gamete_t, mcont_t,
                                typename void_t<
                                    typename mcont_t::value_type>::type>
                : is_rec_model_SFINAE_base<recmodel_t, gamete_t, mcont_t>
            {
            };

            template <typename mcont_t, typename = void> struct mmodel_t
            {
                using type = void;
            };

            template <typename mcont_t>
            struct mmodel_t<mcont_t, typename void_t<
                                         typename mcont_t::value_type>::type>
            {
                using type = typename std::
                    conditional<is_mutation<
                                    typename mcont_t::value_type>::value,
                                std::function<std::size_t(
                                    recycling_bin_t<mcont_t> &, mcont_t &)>,
                                void>::type;
            };

            template <typename mcont_t, typename gcont_t, typename = void>
            struct mmodel_gamete_t
            {
                using type = void;
            };

            template <typename mcont_t, typename gcont_t>
            struct mmodel_gamete_t<mcont_t, gcont_t,
                                   typename void_t<
                                       typename mcont_t::value_type,
                                       typename gcont_t::value_type>::type>
            {
                using type = typename std::
                    conditional<is_mutation<
                                    typename mcont_t::value_type>::value
                                    && is_gamete<typename gcont_t::
                                                     value_type>::value,
                                std::function<std::size_t(
                                    recycling_bin_t<mcont_t> &,
                                    typename gcont_t::value_type &,
                                    mcont_t &)>,
                                void>::type;
            };
        }
    }
}

#endif
