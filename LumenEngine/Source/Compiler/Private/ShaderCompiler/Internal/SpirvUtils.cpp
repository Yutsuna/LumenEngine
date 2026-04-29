/**
 * @file SpirvUtils.cpp
 * @brief Implementation of various SPIR-V maintenance utilities.
 */

#include "ShaderCompiler/Internal/SpirvUtils.hpp"

#include <spirv-tools/libspirv.hpp>

namespace LumenEngine::Compiler::Internal
{

LUMEN_DISABLE_UBSAN FString FSpirvUtils::Disassemble ( const FSpirVBlob &InSpirV ) noexcept
{
    if ( InSpirV.empty() )
    {
        return {};
    }

    spvtools::SpirvTools Tools( SPV_ENV_VULKAN_1_3 );
    FString Disassembly;
    if ( not Tools.Disassemble( InSpirV, &Disassembly, SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES ) )
    {
        return "Failed to disassemble SPIR-V.";
    }

    return Disassembly;
}

LUMEN_DISABLE_UBSAN FString FSpirvUtils::Validate ( const FSpirVBlob &InSpirV ) noexcept
{
    if ( InSpirV.empty() )
    {
        return "SPIR-V blob is empty.";
    }

    spvtools::SpirvTools Tools( SPV_ENV_VULKAN_1_3 );
    FString Diagnostic;
    auto MessageConsumer = [&Diagnostic] ( spv_message_level_t /*Level*/, const AnsiChar * /*Source*/, const spv_position_t &Position, const AnsiChar *Message )
    { Diagnostic += std::format( "at line {}: {}\n", Position.line, Message ); };

    Tools.SetMessageConsumer( MessageConsumer );

    if ( not Tools.Validate( InSpirV ) )
    {
        return Diagnostic;
    }

    return "Validation successful.";
}

} // namespace LumenEngine::Compiler::Internal
