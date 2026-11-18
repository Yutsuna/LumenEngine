/**
 * @file RHIFormatInfo.cpp
 * @brief Implements safe format-table lookup.
 */

#include "RHIFormatTable.hpp"

const Lumen::RHI::FRHIFormatInfo &Lumen::RHI::GetRHIFormatInfo ( const ERHIFormat InFormat ) noexcept
{
    const TSpan<const FRHIFormatInfo> Table = GetRHIFormatTable();
    const USize Index                       = static_cast<USize>( InFormat );
    return Index < Table.size() ? Table[Index] : Table[0];
}

Lumen::TOptional<Lumen::RHI::FRHIFormatInfo> Lumen::RHI::TryGetRHIFormatInfo ( const ERHIFormat InFormat ) noexcept
{
    const TSpan<const FRHIFormatInfo> Table = GetRHIFormatTable();
    const USize Index                       = static_cast<USize>( InFormat );
    if ( Index >= Table.size() )
    {
        return {};
    }
    return TOptional<FRHIFormatInfo>( Table[Index] );
}
