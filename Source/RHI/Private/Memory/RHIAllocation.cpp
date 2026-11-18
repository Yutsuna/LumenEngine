/**
 * @file RHIAllocation.cpp
 * @brief Implements allocation range queries.
 */

#include <Memory/RHIAllocation.hpp>
#include <Utilities/RHIAlignment.hpp>

Lumen::Bool Lumen::FRHIAllocation::Contains ( const UInt64 InOffset, const UInt64 InSize ) const noexcept
{
    return IsValid() and InOffset >= Offset and InSize <= Size and InOffset - Offset <= Size - InSize;
}
