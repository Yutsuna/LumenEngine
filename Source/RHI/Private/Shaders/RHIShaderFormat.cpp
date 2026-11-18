/**
 * @file RHIShaderFormat.cpp
 * @brief Implements shader format validation.
 */

#include <Shaders/RHIShaderFormat.hpp>
Lumen::Bool Lumen::IsKnownRHIShaderFormat ( const ERHIShaderFormat InFormat ) noexcept
{
    return static_cast<UInt8>( InFormat ) < static_cast<UInt8>( ERHIShaderFormat::Count );
}
