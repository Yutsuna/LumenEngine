/**
 * @file SpirvUtils.cpp
 * @brief Implementation of various SPIR-V maintenance utilities.
 */

#include "ShaderCompiler/Internal/SpirvUtils.hpp"

#include <spirv-tools/libspirv.hpp>

#include <format>

LUMEN_DISABLE_UBSAN LumenEngine::FString LumenEngine::Compiler::Internal::FSpirvUtils::Disassemble ( const FSpirVBlob &InSpirV ) noexcept
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

LUMEN_DISABLE_UBSAN LumenEngine::FString LumenEngine::Compiler::Internal::FSpirvUtils::Validate ( const FSpirVBlob &InSpirV ) noexcept
{
    if ( InSpirV.empty() )
    {
        return "SPIR-V blob is empty.";
    }

    spvtools::SpirvTools Tools( SPV_ENV_VULKAN_1_3 );
    FString Diagnostic;

    const spvtools::MessageConsumer MessageConsumer = [&Diagnostic] ( spv_message_level_t /*InLevel*/, const AnsiChar * /*InSource*/,
                                                                      const spv_position_t &InPosition, const AnsiChar *InMessage ) -> void
    { Diagnostic += std::format( "at line {}: {}\n", InPosition.line, InMessage ); };

    Tools.SetMessageConsumer( MessageConsumer );

    if ( not Tools.Validate( InSpirV ) )
    {
        return Diagnostic;
    }

    return "Validation successful.";
}
