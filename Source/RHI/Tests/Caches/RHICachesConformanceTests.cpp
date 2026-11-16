/**
 * @file RHICachesConformanceTests.cpp
 * @brief Runs portable pipeline cache insertion, lookup, and serialization.
 */

#include <Caches/IRHIPipelineCache.hpp>
#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Pipelines/RHIComputePipelineDesc.hpp>
#include <Shaders/IRHIShader.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHICachesConformance, ReusesAndSerializesComputePipelines )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Layout = *( *Device )->CreatePipelineLayout( {} );
        FRHIShaderDesc ShaderDesc;
        ShaderDesc.Stage = ERHIShaderStage::Compute;
        auto Shader      = *( *Device )->CreateShader( ShaderDesc );
        FRHIComputePipelineDesc PipelineDesc;
        PipelineDesc.Layout = Layout;
        PipelineDesc.Shader = Shader;
        auto Pipeline       = ( *Device )->CreateComputePipeline( PipelineDesc );
        LUMEN_CHECK_TRUE( Pipeline.HasValue() );
        auto Cache = ( *Device )->CreatePipelineCache( {} );
        LUMEN_CHECK_TRUE( Cache.HasValue() );
        const auto &Key = ( *Pipeline )->GetCompatibility();
        LUMEN_CHECK_FALSE( ( *Cache )->Find( Key ).IsValid() );
        LUMEN_CHECK_TRUE( ( *Cache )->Insert( Key, *Pipeline ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Cache )->Find( Key ) == *Pipeline );
        LUMEN_CHECK_EQ( ( *Cache )->GetStats().Hits, 1ULL );
        auto Data = ( *Cache )->Serialize();
        LUMEN_CHECK_TRUE( Data.HasValue() );
        LUMEN_CHECK_TRUE( not Data->empty() );
        FRHIPipelineCacheDesc RestoredDesc;
        RestoredDesc.InitialData = *Data;
        auto Restored            = ( *Device )->CreatePipelineCache( RestoredDesc );
        LUMEN_CHECK_TRUE( Restored.HasValue() );
        LUMEN_CHECK_EQ( ( *Restored )->GetStats().EntryCount, 1ULL );
        ( *Restored )->Remove( Key );
        LUMEN_CHECK_EQ( ( *Restored )->GetStats().EntryCount, 0ULL );
    }

} // namespace
} // namespace Lumen
