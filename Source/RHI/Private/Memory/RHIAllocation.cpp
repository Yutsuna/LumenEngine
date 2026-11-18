/**
 * @file RHIAllocation.cpp
 * @brief Implements allocation range queries.
 */

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

Lumen::Bool Lumen::RHI::FRHIAllocation::Contains ( const UInt64 InOffset, const UInt64 InSize ) const noexcept
{
    return IsValid() and InOffset >= Offset and InSize <= Size and InOffset - Offset <= Size - InSize;
}
