/**
 * @file NullPipelinesTests.cpp
 * @brief Tests immutable Null pipeline objects.
 */

#include <Pipelines/NullPipelines.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( NullPipelines, PreserveIdentityTypeAndCompatibility )
        {
            Tests::FNullPipelineLayout Layout( {}, FRHIObjectId{ 1ULL } );
            LUMEN_CHECK_EQ( Layout.GetId().GetValue(), 1ULL );
            LUMEN_CHECK_EQ( Layout.GetHash(), 0ULL );

            FRHIPipelineCompatibility Compatibility;
            Compatibility.LayoutHash       = 9ULL;
            Compatibility.ColorFormats     = { ERHIFormat::RGBA8Unorm };
            Compatibility.VertexLayoutHash = 5ULL;

            Tests::FNullPipeline Pipeline( ERHIPipelineType::Graphics, Compatibility, FRHIObjectId{ 2ULL } );
            LUMEN_CHECK_EQ( Pipeline.GetId().GetValue(), 2ULL );
            LUMEN_CHECK_EQ( Pipeline.GetType(), ERHIPipelineType::Graphics );
            LUMEN_CHECK_TRUE( Pipeline.GetCompatibility() == Compatibility );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
