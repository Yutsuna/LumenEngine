/**
 * @file SpirvUtils.cpp
 * @brief Implementation of the SPIR-V utility backend.
 */

#include "Shader/Internal/SpirvUtils.hpp"

#include <format>
#include <spirv-tools/libspirv.hpp>

LUMEN_DISABLE_UBSAN LumenEngine::FString LumenEngine::Internal::FSpirvUtils::Disassemble ( const FSpirVBlob &InSpirV ) noexcept
{
    spvtools::SpirvTools Tools( SPV_ENV_VULKAN_1_3 );
    FString Assembly;

    if ( Tools.Disassemble( InSpirV.data(), InSpirV.size(), &Assembly ) )
    {
        return Assembly;
    }

    return "";
}

LUMEN_DISABLE_UBSAN LumenEngine::FString LumenEngine::Internal::FSpirvUtils::Validate ( const FSpirVBlob &InSpirV ) noexcept
{
    spvtools::SpirvTools Tools( SPV_ENV_VULKAN_1_3 );
    FString ErrorMsg;

    Tools.SetMessageConsumer( [&ErrorMsg] ( spv_message_level_t /* Level */, const char * /* Source */, const spv_position_t &Pos, const char *Msg )
                              { ErrorMsg += std::format( "Validation error at line {}: {}\n", Pos.line, Msg ); } );

    if ( Tools.Validate( InSpirV.data(), InSpirV.size() ) )
    {
        return "";
    }

    return ErrorMsg;
}
