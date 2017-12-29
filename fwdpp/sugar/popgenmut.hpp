#ifndef __FWDPP_SUGAR_MUTATION_POPGENMUT_HPP__
#define __FWDPP_SUGAR_MUTATION_POPGENMUT_HPP__

#include <fwdpp/forward_types.hpp>
#include <fwdpp/tags/tags.hpp>
#include <fwdpp/io/scalar_serialization.hpp>
#include <fwdpp/io/mutation.hpp>
#include <limits>
#include <tuple>

namespace fwdpp
{
    /*!
      \brief Mutations with selection, dominance, and tracking age of origin
      The "standard" mutation type for population genetic simulation.
      A mutation has its own selection and dominance coefficients.

      \ingroup sugar
     */
    struct popgenmut : public mutation_base
    {
        //! The generation when the mutation arose
        uint_t g;
        //! Selection coefficient
        double s;
        //! Dominance of the mutation
        double h;
        //! Alias for tuple type that can be used for object construction
        using constructor_tuple
            = std::tuple<double, double, double, unsigned, std::uint16_t>;

        /*!
          \brief Constructor
          \param __pos Mutation position
          \param __s Selection coefficient
          \param __h Dominance coefficient
          \param __g Generation when mutation arose
          \param __x Value to assign to mutation_base::xtra
        */
        popgenmut(const double &__pos, const double &__s, const double &__h,
                  const unsigned &__g, const std::uint16_t x = 0) noexcept
            : mutation_base(__pos, (__s == 0.) ? true : false, x), g(__g),
              s(__s), h(__h)
        {
        }

        ///
        /// Construct from a tuple.
        ///
        /// \param t Elements must be pos, s, h, g, x
        ///
        /// \version
        /// Added in fwdpp 0.5.7
        popgenmut(constructor_tuple t) noexcept
            : mutation_base(std::get<0>(t),
                            (std::get<1>(t) == 0.) ? true : false,
                            std::get<4>(t)),
              g(std::get<3>(t)), s(std::get<1>(t)), h(std::get<2>(t))
        {
        }

        bool
        operator==(const popgenmut &rhs) const
        {
            return this->pos == rhs.pos && this->s == rhs.s && this->h == rhs.h
                   && this->g == rhs.g && this->neutral == rhs.neutral;
        }
    };

    namespace io
    {
        template <> struct serialize_mutation<popgenmut>
        {
            io::scalar_writer writer;
            serialize_mutation<popgenmut>() : writer{} {}
            template <typename streamtype>
            inline void
            operator()(const popgenmut &m, streamtype &buffer) const
            {
                writer(buffer, &m.g);
                writer(buffer, &m.pos);
                writer(buffer, &m.s);
                writer(buffer, &m.h);
                writer(buffer, &m.xtra);
            }
        };

        template <> struct deserialize_mutation<popgenmut>
        {
            io::scalar_reader reader;
            deserialize_mutation<popgenmut>() : reader{} {}
            template <typename streamtype>
            inline popgenmut
            operator()(streamtype &buffer) const
            {
                std::size_t g;
                double pos, s, h;
                decltype(popgenmut::xtra) xtra;
                io::scalar_reader reader;
                reader(buffer, &g);
                reader(buffer, &pos);
                reader(buffer, &s);
                reader(buffer, &h);
                reader(buffer, &xtra);
                return popgenmut(pos, s, h, g, xtra);
            }
        };
    }
}
#endif
