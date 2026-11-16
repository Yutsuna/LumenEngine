/**
 * @file RHIShadersConformanceTests.cpp
 * @brief Runs portable shader creation scenarios.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Shaders/IRHIShader.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIShadersConformance, CreatesShaderAndRejectsInvalidDescriptor )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TArray<Byte, 4U> Bytecode = { 1U, 2U, 3U, 4U };
        FRHIShaderDesc Desc;
        Desc.Bytecode = TSpan<const Byte>( Bytecode.data(), Bytecode.size() );
        Desc.Stage    = ERHIShaderStage::Vertex;
        Desc.Format   = ERHIShaderFormat::SPIRV;
        auto Shader   = ( *Device )->CreateShader( Desc );
        LUMEN_CHECK_TRUE( Shader.HasValue() );
        LUMEN_CHECK_EQ( ( *Shader )->GetDesc().Stage, ERHIShaderStage::Vertex );
        LUMEN_CHECK_EQ( ( *Shader )->GetDesc().Bytecode.size(), Bytecode.size() );
        Desc.Stage = ERHIShaderStage::None;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateShader( Desc ) );
    }

} // namespace
} // namespace Lumen
