/**
 * @file RHIIdentifiers.cpp
 * @brief Implements the monotonic identifier generator.
 */

#include <RHI/Types/RHIIdentifiers.hpp>

Lumen::RHI::FRHIIdentifierGenerator::FRHIIdentifierGenerator () noexcept : NextValue( 1ULL )
{
}

Lumen::RHI::FRHIIdentifierGenerator::FRHIIdentifierGenerator ( const UInt64 InFirstValue ) noexcept : NextValue( InFirstValue )
{
}
