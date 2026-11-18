/**
 * @file RHIShaderDescTests.cpp
 * @brief Tests shader descriptor validation.
 */

#include <Shaders/RHIShaderDesc.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIShaderDesc, RequiresSingleStageEntryPointAndKnownFormat )
    {
        const TArray<Byte, 4U> Bytecode = { 1U, 2U, 3U, 4U };
        FRHIShaderDesc Desc;
        Desc.Bytecode = TSpan<const Byte>( Bytecode.data(), Bytecode.size() );
        Desc.Stage    = ERHIShaderStage::Vertex;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.EntryPoint.clear();
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.EntryPoint = "Main";
        Desc.Stage      = ERHIShaderStage::Vertex | ERHIShaderStage::Pixel;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Stage  = ERHIShaderStage::Vertex;
        Desc.Format = static_cast<ERHIShaderFormat>( 255U );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

    LUMEN_TEST( RHIShaderDesc, PropagatesInvalidReflection )
    {
        FRHIShaderDesc Desc;
        Desc.Stage = ERHIShaderStage::Compute;
        Desc.Reflection.Bindings.push_back( { .Count = 0U } );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace
} // namespace Lumen
