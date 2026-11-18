/**
 * @file RHIShadersConformanceTests.cpp
 * @brief Runs portable shader creation scenarios.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Shaders/IRHIShader.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIShadersConformance, CreatesShaderAndRejectsInvalidDescriptor )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TExpected<TSharedRef<IRHIShader>, FRHIError> Shader =
            Tests::GetRHIConformanceDeviceFactory().CreateShader( **Device, ERHIShaderStage::Vertex );
        LUMEN_CHECK_TRUE( Shader.HasValue() );
        LUMEN_CHECK_EQ( ( *Shader )->GetDesc().Stage, ERHIShaderStage::Vertex );
        LUMEN_CHECK_TRUE( ( *Shader )->GetId().IsValid() );

        FRHIShaderDesc InvalidDesc                                      = ( *Shader )->GetDesc();
        InvalidDesc.Stage                                               = ERHIShaderStage::None;
        const TExpected<TSharedRef<IRHIShader>, FRHIError> InvalidStage = ( *Device )->CreateShader( InvalidDesc );
        LUMEN_CHECK_HAS_ERROR( InvalidStage );
        LUMEN_CHECK_EQ( InvalidStage.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( InvalidStage.Error().Condition, ERHIErrorCondition::Shader );
    }

    LUMEN_TEST( RHIShadersConformance, RejectsEmptyEntryPointsAndUnknownFormats )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TExpected<TSharedRef<IRHIShader>, FRHIError> Shader =
            Tests::GetRHIConformanceDeviceFactory().CreateShader( **Device, ERHIShaderStage::Compute );
        LUMEN_CHECK_TRUE( Shader.HasValue() );

        FRHIShaderDesc Desc                                                = ( *Shader )->GetDesc();
        Desc.EntryPoint                                                    = "";
        const TExpected<TSharedRef<IRHIShader>, FRHIError> EmptyEntryPoint = ( *Device )->CreateShader( Desc );
        LUMEN_CHECK_HAS_ERROR( EmptyEntryPoint );
        LUMEN_CHECK_EQ( EmptyEntryPoint.Error().Condition, ERHIErrorCondition::Shader );

        Desc                                                      = ( *Shader )->GetDesc();
        Desc.Format                                               = static_cast<ERHIShaderFormat>( 255U );
        const TExpected<TSharedRef<IRHIShader>, FRHIError> Format = ( *Device )->CreateShader( Desc );
        LUMEN_CHECK_HAS_ERROR( Format );
        LUMEN_CHECK_EQ( Format.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Format.Error().Condition, ERHIErrorCondition::Shader );
    }

} // namespace
} // namespace Lumen
