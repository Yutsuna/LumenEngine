/**
 * @file RHIEnumToString.cpp
 * @brief Implements stable textual names for RHI enums.
 */

#include <RHI/Utilities/RHIEnumToString.hpp>

#include <Meta/Reflection.hpp>

namespace
{
/** @brief Reuse Core reflection for the qualified name, keeping RHI's "Unknown" fallback contract. */
template <typename TEnum> [[nodiscard]] Lumen::FStringView RHIEnumName ( const TEnum InValue ) noexcept
{
    const Lumen::FStringView Name = Lumen::Meta::EnumQualifiedName( InValue );
    return Name == "<unknown>" ? Lumen::FStringView( "Unknown" ) : Name;
}
} // namespace

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIFormat InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIErrorCode InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIErrorCondition InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIQueueType InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIResourceState InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHISampleCount InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIShaderStage InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHICapabilityFlags InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIPresentMode InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIColorSpace InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHISurfaceType InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIQueryType InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIDebugSeverity InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHIDebugCategory InValue ) noexcept
{
    return RHIEnumName( InValue );
}

Lumen::FStringView Lumen::RHI::ToRHIString ( const ERHICacheEvictionPolicy InValue ) noexcept
{
    return RHIEnumName( InValue );
}
