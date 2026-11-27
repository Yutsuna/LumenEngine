/**
 * @file RHIShaderFormat.cpp
 * @brief Implements shader format validation.
 */

#include <RHI/Types/RHIShaderFormat.hpp>
Lumen::Bool Lumen::RHI::IsKnownRHIShaderFormat ( const ERHIShaderFormat InFormat ) noexcept
{
    return static_cast<UInt8>( InFormat ) < static_cast<UInt8>( ERHIShaderFormat::Count );
}
