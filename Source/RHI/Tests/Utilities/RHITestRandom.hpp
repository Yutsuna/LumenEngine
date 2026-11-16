/**
 * @file RHITestRandom.hpp
 * @brief Declares deterministic random support for RHI tests.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Tests
{

    /**
     * @class FRHITestRandom
     * @brief Reproducible SplitMix64 generator with bounded ranges.
     */
    class FRHITestRandom final
    {
    public:

        /** @brief Starts a reproducible sequence. */
        constexpr explicit FRHITestRandom ( const UInt64 InSeed = 0x4C554D454EULL ) noexcept : State( InSeed )
        {
        }

        /** @return Next raw random value. */
        constexpr UInt64 Next () noexcept
        {
            State += 0x9E3779B97F4A7C15ULL;
            UInt64 Value = State;
            Value        = ( Value ^ ( Value >> 30U ) ) * 0xBF58476D1CE4E5B9ULL;
            Value        = ( Value ^ ( Value >> 27U ) ) * 0x94D049BB133111EBULL;
            return Value ^ ( Value >> 31U );
        }

        /** @return Random value in [0, InUpperBound); zero for an empty range. */
        constexpr UInt64 Next ( const UInt64 InUpperBound ) noexcept
        {
            return InUpperBound == 0ULL ? 0ULL : Next() % InUpperBound;
        }

    private:

        UInt64 State;
    };

} // namespace Tests

} // namespace Lumen
