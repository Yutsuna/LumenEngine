/**
 * @file RHIIdentifiers.cpp
 * @brief Implements the monotonic identifier generator.
 */

#include <Types/RHIIdentifiers.hpp>

Lumen::FRHIIdentifierGenerator::FRHIIdentifierGenerator () noexcept : NextValue( 1ULL )
{
}
