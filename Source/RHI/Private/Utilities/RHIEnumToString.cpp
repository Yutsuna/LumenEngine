/**
 * @file RHIEnumToString.cpp
 * @brief Implements stable textual names for RHI enums.
 */

#include <Utilities/RHIEnumToString.hpp>

#define RHI_ENUM_CASE( Value )                                                                                                   \
    case Value:                                                                                                                  \
        return #Value

Lumen::FStringView Lumen::ToRHIString ( const ERHIFormat InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIFormat::Unknown );
        RHI_ENUM_CASE( ERHIFormat::R8Unorm );
        RHI_ENUM_CASE( ERHIFormat::R8Snorm );
        RHI_ENUM_CASE( ERHIFormat::R8Uint );
        RHI_ENUM_CASE( ERHIFormat::R8Sint );
        RHI_ENUM_CASE( ERHIFormat::RG8Unorm );
        RHI_ENUM_CASE( ERHIFormat::RG8Snorm );
        RHI_ENUM_CASE( ERHIFormat::RG8Uint );
        RHI_ENUM_CASE( ERHIFormat::RG8Sint );
        RHI_ENUM_CASE( ERHIFormat::RGBA8Unorm );
        RHI_ENUM_CASE( ERHIFormat::RGBA8Snorm );
        RHI_ENUM_CASE( ERHIFormat::RGBA8Uint );
        RHI_ENUM_CASE( ERHIFormat::RGBA8Sint );
        RHI_ENUM_CASE( ERHIFormat::RGBA8Srgb );
        RHI_ENUM_CASE( ERHIFormat::BGRA8Unorm );
        RHI_ENUM_CASE( ERHIFormat::BGRA8Srgb );
        RHI_ENUM_CASE( ERHIFormat::R16Unorm );
        RHI_ENUM_CASE( ERHIFormat::R16Snorm );
        RHI_ENUM_CASE( ERHIFormat::R16Uint );
        RHI_ENUM_CASE( ERHIFormat::R16Sint );
        RHI_ENUM_CASE( ERHIFormat::R16Float );
        RHI_ENUM_CASE( ERHIFormat::RG16Unorm );
        RHI_ENUM_CASE( ERHIFormat::RG16Snorm );
        RHI_ENUM_CASE( ERHIFormat::RG16Uint );
        RHI_ENUM_CASE( ERHIFormat::RG16Sint );
        RHI_ENUM_CASE( ERHIFormat::RG16Float );
        RHI_ENUM_CASE( ERHIFormat::RGBA16Unorm );
        RHI_ENUM_CASE( ERHIFormat::RGBA16Snorm );
        RHI_ENUM_CASE( ERHIFormat::RGBA16Uint );
        RHI_ENUM_CASE( ERHIFormat::RGBA16Sint );
        RHI_ENUM_CASE( ERHIFormat::RGBA16Float );
        RHI_ENUM_CASE( ERHIFormat::R32Uint );
        RHI_ENUM_CASE( ERHIFormat::R32Sint );
        RHI_ENUM_CASE( ERHIFormat::R32Float );
        RHI_ENUM_CASE( ERHIFormat::RG32Uint );
        RHI_ENUM_CASE( ERHIFormat::RG32Sint );
        RHI_ENUM_CASE( ERHIFormat::RG32Float );
        RHI_ENUM_CASE( ERHIFormat::RGB32Uint );
        RHI_ENUM_CASE( ERHIFormat::RGB32Sint );
        RHI_ENUM_CASE( ERHIFormat::RGB32Float );
        RHI_ENUM_CASE( ERHIFormat::RGBA32Uint );
        RHI_ENUM_CASE( ERHIFormat::RGBA32Sint );
        RHI_ENUM_CASE( ERHIFormat::RGBA32Float );
        RHI_ENUM_CASE( ERHIFormat::D16Unorm );
        RHI_ENUM_CASE( ERHIFormat::D24UnormS8Uint );
        RHI_ENUM_CASE( ERHIFormat::D32Float );
        RHI_ENUM_CASE( ERHIFormat::D32FloatS8Uint );
        RHI_ENUM_CASE( ERHIFormat::BC1RGBAUnorm );
        RHI_ENUM_CASE( ERHIFormat::BC1RGBASrgb );
        RHI_ENUM_CASE( ERHIFormat::BC3RGBAUnorm );
        RHI_ENUM_CASE( ERHIFormat::BC3RGBASrgb );
        RHI_ENUM_CASE( ERHIFormat::BC7RGBAUnorm );
        RHI_ENUM_CASE( ERHIFormat::BC7RGBASrgb );
        RHI_ENUM_CASE( ERHIFormat::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIErrorCode InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIErrorCode::InvalidArgument );
        RHI_ENUM_CASE( ERHIErrorCode::InvalidState );
        RHI_ENUM_CASE( ERHIErrorCode::Unsupported );
        RHI_ENUM_CASE( ERHIErrorCode::OutOfMemory );
        RHI_ENUM_CASE( ERHIErrorCode::DeviceLost );
        RHI_ENUM_CASE( ERHIErrorCode::DeviceClosed );
        RHI_ENUM_CASE( ERHIErrorCode::OutOfDate );
        RHI_ENUM_CASE( ERHIErrorCode::Timeout );
        RHI_ENUM_CASE( ERHIErrorCode::NotFound );
        RHI_ENUM_CASE( ERHIErrorCode::AlreadyExists );
        RHI_ENUM_CASE( ERHIErrorCode::ValidationFailed );
        RHI_ENUM_CASE( ERHIErrorCode::CorruptedData );
        RHI_ENUM_CASE( ERHIErrorCode::BackendFailure );
        RHI_ENUM_CASE( ERHIErrorCode::InternalFailure );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIErrorCondition InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIErrorCondition::None );
        RHI_ENUM_CASE( ERHIErrorCondition::Capability );
        RHI_ENUM_CASE( ERHIErrorCondition::Limit );
        RHI_ENUM_CASE( ERHIErrorCondition::Queue );
        RHI_ENUM_CASE( ERHIErrorCondition::Resource );
        RHI_ENUM_CASE( ERHIErrorCondition::Memory );
        RHI_ENUM_CASE( ERHIErrorCondition::Binding );
        RHI_ENUM_CASE( ERHIErrorCondition::Shader );
        RHI_ENUM_CASE( ERHIErrorCondition::Pipeline );
        RHI_ENUM_CASE( ERHIErrorCondition::Command );
        RHI_ENUM_CASE( ERHIErrorCondition::Synchronization );
        RHI_ENUM_CASE( ERHIErrorCondition::Presentation );
        RHI_ENUM_CASE( ERHIErrorCondition::Query );
        RHI_ENUM_CASE( ERHIErrorCondition::Cache );
        RHI_ENUM_CASE( ERHIErrorCondition::Device );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIQueueType InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIQueueType::Graphics );
        RHI_ENUM_CASE( ERHIQueueType::Compute );
        RHI_ENUM_CASE( ERHIQueueType::Copy );
        RHI_ENUM_CASE( ERHIQueueType::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIResourceState InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIResourceState::Undefined );
        RHI_ENUM_CASE( ERHIResourceState::Common );
        RHI_ENUM_CASE( ERHIResourceState::VertexBuffer );
        RHI_ENUM_CASE( ERHIResourceState::IndexBuffer );
        RHI_ENUM_CASE( ERHIResourceState::ConstantBuffer );
        RHI_ENUM_CASE( ERHIResourceState::ShaderResource );
        RHI_ENUM_CASE( ERHIResourceState::UnorderedAccess );
        RHI_ENUM_CASE( ERHIResourceState::RenderTarget );
        RHI_ENUM_CASE( ERHIResourceState::DepthWrite );
        RHI_ENUM_CASE( ERHIResourceState::DepthRead );
        RHI_ENUM_CASE( ERHIResourceState::CopySource );
        RHI_ENUM_CASE( ERHIResourceState::CopyDestination );
        RHI_ENUM_CASE( ERHIResourceState::ResolveSource );
        RHI_ENUM_CASE( ERHIResourceState::ResolveDestination );
        RHI_ENUM_CASE( ERHIResourceState::IndirectArgument );
        RHI_ENUM_CASE( ERHIResourceState::Present );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHISampleCount InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHISampleCount::Count1 );
        RHI_ENUM_CASE( ERHISampleCount::Count2 );
        RHI_ENUM_CASE( ERHISampleCount::Count4 );
        RHI_ENUM_CASE( ERHISampleCount::Count8 );
        RHI_ENUM_CASE( ERHISampleCount::Count16 );
        RHI_ENUM_CASE( ERHISampleCount::Count32 );
        RHI_ENUM_CASE( ERHISampleCount::Count64 );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIShaderStage InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIShaderStage::None );
        RHI_ENUM_CASE( ERHIShaderStage::Vertex );
        RHI_ENUM_CASE( ERHIShaderStage::Pixel );
        RHI_ENUM_CASE( ERHIShaderStage::Compute );
        RHI_ENUM_CASE( ERHIShaderStage::Geometry );
        RHI_ENUM_CASE( ERHIShaderStage::Hull );
        RHI_ENUM_CASE( ERHIShaderStage::Domain );
        RHI_ENUM_CASE( ERHIShaderStage::Amplification );
        RHI_ENUM_CASE( ERHIShaderStage::Mesh );
        RHI_ENUM_CASE( ERHIShaderStage::AllGraphics );
        RHI_ENUM_CASE( ERHIShaderStage::All );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHICapabilityFlags InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHICapabilityFlags::None );
        RHI_ENUM_CASE( ERHICapabilityFlags::DynamicRendering );
        RHI_ENUM_CASE( ERHICapabilityFlags::TimelineSemaphores );
        RHI_ENUM_CASE( ERHICapabilityFlags::BindlessResources );
        RHI_ENUM_CASE( ERHICapabilityFlags::IndirectDraw );
        RHI_ENUM_CASE( ERHICapabilityFlags::MultiDrawIndirect );
        RHI_ENUM_CASE( ERHICapabilityFlags::TimestampQueries );
        RHI_ENUM_CASE( ERHICapabilityFlags::OcclusionQueries );
        RHI_ENUM_CASE( ERHICapabilityFlags::PipelineStatistics );
        RHI_ENUM_CASE( ERHICapabilityFlags::MeshShaders );
        RHI_ENUM_CASE( ERHICapabilityFlags::RayTracing );
        RHI_ENUM_CASE( ERHICapabilityFlags::VariableRateShading );
        RHI_ENUM_CASE( ERHICapabilityFlags::ConservativeRasterization );
        RHI_ENUM_CASE( ERHICapabilityFlags::DepthClamp );
        RHI_ENUM_CASE( ERHICapabilityFlags::AnisotropicFiltering );
        RHI_ENUM_CASE( ERHICapabilityFlags::AsyncCompute );
        RHI_ENUM_CASE( ERHICapabilityFlags::Presentation );
        RHI_ENUM_CASE( ERHICapabilityFlags::GeometryShaders );
        RHI_ENUM_CASE( ERHICapabilityFlags::Tessellation );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIPresentMode InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIPresentMode::Fifo );
        RHI_ENUM_CASE( ERHIPresentMode::Immediate );
        RHI_ENUM_CASE( ERHIPresentMode::Mailbox );
        RHI_ENUM_CASE( ERHIPresentMode::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIColorSpace InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIColorSpace::SrgbNonLinear );
        RHI_ENUM_CASE( ERHIColorSpace::DisplayP3NonLinear );
        RHI_ENUM_CASE( ERHIColorSpace::Hdr10St2084 );
        RHI_ENUM_CASE( ERHIColorSpace::ExtendedSrgbLinear );
        RHI_ENUM_CASE( ERHIColorSpace::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHISurfaceType InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHISurfaceType::None );
        RHI_ENUM_CASE( ERHISurfaceType::Null );
        RHI_ENUM_CASE( ERHISurfaceType::Win32 );
        RHI_ENUM_CASE( ERHISurfaceType::Xlib );
        RHI_ENUM_CASE( ERHISurfaceType::Xcb );
        RHI_ENUM_CASE( ERHISurfaceType::Wayland );
        RHI_ENUM_CASE( ERHISurfaceType::Metal );
        RHI_ENUM_CASE( ERHISurfaceType::Android );
        RHI_ENUM_CASE( ERHISurfaceType::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIQueryType InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIQueryType::Occlusion );
        RHI_ENUM_CASE( ERHIQueryType::Timestamp );
        RHI_ENUM_CASE( ERHIQueryType::PipelineStatistics );
        RHI_ENUM_CASE( ERHIQueryType::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIDebugSeverity InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIDebugSeverity::Info );
        RHI_ENUM_CASE( ERHIDebugSeverity::Warning );
        RHI_ENUM_CASE( ERHIDebugSeverity::Error );
        RHI_ENUM_CASE( ERHIDebugSeverity::Fatal );
        RHI_ENUM_CASE( ERHIDebugSeverity::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHIDebugCategory InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHIDebugCategory::General );
        RHI_ENUM_CASE( ERHIDebugCategory::Validation );
        RHI_ENUM_CASE( ERHIDebugCategory::Performance );
        RHI_ENUM_CASE( ERHIDebugCategory::Resource );
        RHI_ENUM_CASE( ERHIDebugCategory::Synchronization );
        RHI_ENUM_CASE( ERHIDebugCategory::Presentation );
        RHI_ENUM_CASE( ERHIDebugCategory::Pipeline );
        RHI_ENUM_CASE( ERHIDebugCategory::Count );
    }
    return "Unknown";
}

Lumen::FStringView Lumen::ToRHIString ( const ERHICacheEvictionPolicy InValue ) noexcept
{
    switch ( InValue )
    {
        RHI_ENUM_CASE( ERHICacheEvictionPolicy::None );
        RHI_ENUM_CASE( ERHICacheEvictionPolicy::LeastRecentlyUsed );
    }
    return "Unknown";
}

#undef RHI_ENUM_CASE
