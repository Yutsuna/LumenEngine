/**
 * @file RHIIdentifiers.cpp
 * @brief Implements the monotonic identifier generator.
 */

#include <Types/RHIIdentifiers.hpp>

Lumen::FRHIIdentifierGenerator::FRHIIdentifierGenerator () noexcept : NextValue( 1ULL )
{
}

Lumen::FRHIIdentifierGenerator::FRHIIdentifierGenerator ( const UInt64 InFirstValue ) noexcept : NextValue( InFirstValue )
{
}
