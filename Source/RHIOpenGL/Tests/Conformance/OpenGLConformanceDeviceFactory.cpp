/**
 * @file OpenGLConformanceDeviceFactory.cpp
 * @brief Implements the OpenGL conformance device factory.
 */

#include "Conformance/OpenGLConformanceDeviceFactory.hpp"

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Device/OpenGLDevice.hpp>
#include <RHIOpenGL/Module/OpenGLModule.hpp>
#include <RHIOpenGL/Presentation/OpenGLSwapchain.hpp>

#include <Templates/Optional.hpp>

/**
 * File-local helpers
 */

namespace
{

/** @return A minimal, valid GLSL 460 source module for the requested stage. */
[[nodiscard]] Lumen::TVector<Lumen::Byte> MakeShaderSource ( const Lumen::RHI::ERHIShaderStage InStage ) noexcept
{
    using Lumen::RHI::ERHIShaderStage;
    Lumen::FStringView Source;
    switch ( InStage )
    {
    case ERHIShaderStage::Vertex:
        Source = "#version 460 core\n"
                 "layout(location = 0) in vec3 inPosition;\n"
                 "layout(location = 1) in vec2 inTexCoord;\n"
                 "layout(binding = 1) uniform UniformBlock1 { mat4 transform; };\n"
                 "void main() { gl_Position = transform * vec4(inPosition, 1.0); }\n";
        break;
    case ERHIShaderStage::Pixel:
        Source = "#version 460 core\n"
                 "layout(location = 0) out vec4 outColor;\n"
                 "layout(binding = 1) uniform UniformBlock1 { mat4 transform; };\n"
                 "void main() { outColor = transform[3]; }\n";
        break;
    case ERHIShaderStage::Compute:
        Source = "#version 460 core\n"
                 "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
                 "layout(binding = 1) uniform UniformBlock1 { mat4 transform; };\n"
                 "void main() { mat4 observed = transform; if (observed[0][0] == 0.0) { return; } }\n";
        break;
    default:
        return {};
    }
    return Lumen::TVector<Lumen::Byte>( reinterpret_cast<const Lumen::Byte *>( Source.data() ),
                                        reinterpret_cast<const Lumen::Byte *>( Source.data() + Source.size() ) );
}

} // namespace

/**
 * Public
 */

Lumen::TSharedRef<Lumen::RHI::IRHIModule> Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::GetModule () const noexcept
{
    return MakeShared<FOpenGLModule>();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::CreateDevice ( const Bool bEnableValidation ) noexcept
{
    RHI::FRHIDeviceDesc Desc;
    Desc.Queues = { { RHI::ERHIQueueType::Graphics, 1U }, { RHI::ERHIQueueType::Compute, 1U }, { RHI::ERHIQueueType::Copy, 1U } };
    TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = GetModule()->CreateDevice( Desc );
    if ( not Device )
    {
        return Device;
    }
    if ( not bEnableValidation )
    {
        return Device;
    }
    return RHI::WrapRHIDevice( *Device );
}

void Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::SetDefaultValidation ( const Bool bEnableValidation ) noexcept
{
    bDefaultValidation = bEnableValidation;
}

Lumen::Bool Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::GetDefaultValidation () const noexcept
{
    return bDefaultValidation;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::CreateShader ( RHI::IRHIDevice &InDevice,
                                                                         const RHI::ERHIShaderStage InStage ) const noexcept
{
    TVector<Byte> Source = MakeShaderSource( InStage );
    if ( Source.empty() )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Shader,
                                                     "The requested shader stage is not available in OpenGL." ) );
    }
    RHI::FRHIShaderDesc Desc;
    Desc.Stage    = InStage;
    Desc.Format   = RHI::ERHIShaderFormat::Source;
    Desc.Bytecode = TSpan<const Byte>( Source.data(), Source.size() );
    Desc.Reflection.Bindings.push_back(
        { .Slot = 1U, .Kind = RHI::ERHIShaderBindingKind::ConstantBuffer, .Count = 1U, .Stages = InStage } );
    return InDevice.CreateShader( Desc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::CreateSwapchain (
    RHI::IRHIDevice &InDevice, const TSharedPtr<RHI::IRHISwapchain> &InOldSwapchain ) const noexcept
{
    RHI::FRHISwapchainDesc Desc;
    Desc.Surface.Type          = RHI::ERHISurfaceType::Null;
    Desc.Surface.NativeWindow  = 1U;
    Desc.Surface.InitialExtent = { 320U, 200U, 1U };
    Desc.Extent                = Desc.Surface.InitialExtent;
    Desc.OldSwapchain          = InOldSwapchain;
    return InDevice.CreateSwapchain( Desc );
}

Lumen::Bool
Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::InjectDeviceLost ( RHI::IRHIDevice &InDevice ) const noexcept
{
    return RHI::TryUnwrapRHIBackend<FOpenGLDevice>( &InDevice )
        .Inspect( [] ( FOpenGLDevice *Device ) noexcept
    {
        Device->InjectDeviceLost();
    } ).HasValue();
}

Lumen::Bool Lumen::RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory::InjectSwapchainOutOfDate (
    RHI::IRHISwapchain &InSwapchain ) const noexcept
{
    return RHI::TryUnwrapRHIBackend<FOpenGLSwapchain>( &InSwapchain )
        .Inspect( [] ( FOpenGLSwapchain *Swapchain ) noexcept
    {
        Swapchain->MarkOutOfDate();
    } ).HasValue();
}

Lumen::RHI::Tests::IRHIConformanceDeviceFactory &Lumen::RHI::Tests::GetRHIConformanceDeviceFactory () noexcept
{
    static RHIOpenGL::Tests::FOpenGLConformanceDeviceFactory Factory;
    return Factory;
}
