/**
 * @file RHICachesConformanceTests.cpp
 * @brief Runs portable pipeline cache insertion, lookup, and serialization.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Caches/IRHIPipelineCache.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Shaders/IRHIShader.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHICachesConformance, ReusesAndSerializesComputePipelines )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TSharedRef<IRHIPipelineLayout> Layout = *( *Device )->CreatePipelineLayout( {} );
            const TSharedRef<IRHIShader> Shader =
                *Tests::GetRHIConformanceDeviceFactory().CreateShader( **Device, ERHIShaderStage::Compute );
            FRHIComputePipelineDesc PipelineDesc;
            PipelineDesc.Layout                                     = Layout;
            PipelineDesc.Shader                                     = Shader;
            TExpected<TSharedRef<IRHIPipeline>, FRHIError> Pipeline = ( *Device )->CreateComputePipeline( PipelineDesc );
            LUMEN_CHECK_TRUE( Pipeline.HasValue() );
            TExpected<TSharedRef<IRHIPipelineCache>, FRHIError> Cache = ( *Device )->CreatePipelineCache( {} );
            LUMEN_CHECK_TRUE( Cache.HasValue() );
            const FRHIPipelineCacheKey Key = MakeRHIComputePipelineCacheKey( PipelineDesc, ( *Cache )->GetDesc().BackendVersion );
            LUMEN_CHECK_FALSE( ( *Cache )->Find( Key ).HasValue() );
            LUMEN_CHECK_TRUE( ( *Cache )->Insert( Key, *Pipeline ).HasValue() );
            const TOptional<TSharedPtr<IRHIPipeline>> Found = ( *Cache )->Find( Key );
            LUMEN_CHECK_TRUE( Found.HasValue() );
            LUMEN_CHECK_TRUE( *Found == *Pipeline );
            LUMEN_CHECK_EQ( ( *Cache )->GetStats().Hits, 1ULL );
            TExpected<TVector<Byte>, FRHIError> Data = ( *Cache )->Serialize();
            LUMEN_CHECK_TRUE( Data.HasValue() );
            LUMEN_CHECK_TRUE( not Data->empty() );
            FRHIPipelineCacheDesc RestoredDesc;
            RestoredDesc.InitialData                                     = *Data;
            TExpected<TSharedRef<IRHIPipelineCache>, FRHIError> Restored = ( *Device )->CreatePipelineCache( RestoredDesc );
            LUMEN_CHECK_TRUE( Restored.HasValue() );
            LUMEN_CHECK_EQ( ( *Restored )->GetStats().EntryCount, 1ULL );
            ( *Restored )->Remove( Key );
            LUMEN_CHECK_EQ( ( *Restored )->GetStats().EntryCount, 0ULL );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
