/**
 * @file NullShadersTests.cpp
 * @brief Tests Null shader ownership and identity.
 */

#include <Shaders/NullShaders.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( NullShaders, CopiesBytecodeAndKeepsDescriptor )
        {
            TVector<Byte> Bytecode = { 1U, 2U, 3U, 4U };
            FRHIShaderDesc Desc;
            Desc.Bytecode   = TSpan<const Byte>( Bytecode.data(), Bytecode.size() );
            Desc.Stage      = ERHIShaderStage::Compute;
            Desc.EntryPoint = "ComputeMain";
            Desc.Format     = ERHIShaderFormat::PortableBytecode;
            Tests::FNullShader Shader( Desc, FRHIObjectId{ 7ULL } );
            Bytecode[0] = 99U;
            LUMEN_CHECK_EQ( Shader.GetId().GetValue(), 7ULL );
            LUMEN_CHECK_EQ( Shader.GetDesc().Bytecode[0], 1U );
            LUMEN_CHECK_EQ( Shader.GetDesc().EntryPoint, FString( "ComputeMain" ) );
            LUMEN_CHECK_EQ( Shader.GetDesc().Format, ERHIShaderFormat::PortableBytecode );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
