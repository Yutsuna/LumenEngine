/**
 * @file RenderAssetRegistryTests.cpp
 * @brief Functional coverage for control-time geometry asset registration on Null RHI.
 */

#include <Frame/RenderAssetRegistry.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Maths/Vec.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /** @brief Creates the deterministic Null device used by these functional tests. */
    [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> MakeTestDevice () noexcept
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        Factory.SetDefaultValidation( false );
        return Factory.CreateDevice();
    }

    /** @brief Requests the always-present graphics queue. */
    [[nodiscard]] TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> MakeTestQueue ( RHI::IRHIDevice &InDevice ) noexcept
    {
        return InDevice.GetQueue( RHI::ERHIQueueType::Graphics, 0U );
    }

    /** @brief A three-vertex triangle with one triangle's worth of indices. */
    [[nodiscard]] TArray<Maths::FVec3f, 3> MakeTrianglePositions () noexcept
    {
        return { Maths::FVec3f{ 0.0F, 0.0F, 0.0F }, Maths::FVec3f{ 1.0F, 0.0F, 0.0F }, Maths::FVec3f{ 0.0F, 1.0F, 0.0F } };
    }

    /** @brief A four-vertex quad with two triangles' worth of indices. */
    [[nodiscard]] TArray<Maths::FVec3f, 4> MakeQuadPositions () noexcept
    {
        return { Maths::FVec3f{ 0.0F, 0.0F, 0.0F }, Maths::FVec3f{ 1.0F, 0.0F, 0.0F }, Maths::FVec3f{ 1.0F, 1.0F, 0.0F },
                 Maths::FVec3f{ 0.0F, 1.0F, 0.0F } };
    }

    /** @brief Builds a descriptor over borrowed arrays that outlive the Register call. */
    template <typename PositionArray, typename IndexArray>
    [[nodiscard]] FRenderGeometryAssetDesc
    MakeDesc ( const UInt64 InId, const PositionArray &InPositions, const IndexArray &InIndices ) noexcept
    {
        return FRenderGeometryAssetDesc{ .Id        = FRenderAssetId( InId ),
                                         .Positions = TSpan<const Maths::FVec3f>( InPositions.data(), InPositions.size() ),
                                         .Indices   = TSpan<const UInt32>( InIndices.data(), InIndices.size() ) };
    }

    /** @brief True when InUsage contains every bit of InRequired. */
    [[nodiscard]] Bool HasAllUsage ( const RHI::ERHIResourceUsage InUsage, const RHI::ERHIResourceUsage InRequired ) noexcept
    {
        return ( InUsage & InRequired ) == InRequired;
    }

} // namespace

LUMEN_TEST( FRenderAssetRegistry, RegistersTriangleAndQuadWithExpectedBuffers )
{
    const auto Device = MakeTestDevice();
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }
    const auto Queue = MakeTestQueue( **Device );
    LUMEN_CHECK_HAS_VALUE( Queue );
    if ( not Queue )
    {
        return;
    }

    FRenderAssetRegistry Registry( 4ULL );
    const TArray<Maths::FVec3f, 3> TrianglePositions = MakeTrianglePositions();
    const TArray<UInt32, 3> TriangleIndices{ 0U, 1U, 2U };
    const TArray<Maths::FVec3f, 4> QuadPositions = MakeQuadPositions();
    const TArray<UInt32, 6> QuadIndices{ 0U, 1U, 2U, 0U, 2U, 3U };

    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 1ULL, TrianglePositions, TriangleIndices ) ) );
    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 2ULL, QuadPositions, QuadIndices ) ) );
    LUMEN_CHECK_EQ( Registry.GetSize(), 2ULL );
    LUMEN_CHECK_EQ( Registry.GetCapacity(), 4ULL );
    LUMEN_CHECK_EQ( Registry.GetAssets().size(), 2ULL );

    const FRenderGeometryAsset &Triangle = Registry.GetAssets()[0];
    const FRenderGeometryAsset &Quad     = Registry.GetAssets()[1];
    LUMEN_CHECK_EQ( Triangle.Id, FRenderAssetId( 1ULL ) );
    LUMEN_CHECK_EQ( Quad.Id, FRenderAssetId( 2ULL ) );
    LUMEN_CHECK_EQ( Triangle.VertexCount, 3U );
    LUMEN_CHECK_EQ( Triangle.IndexCount, 3U );
    LUMEN_CHECK_EQ( Quad.VertexCount, 4U );
    LUMEN_CHECK_EQ( Quad.IndexCount, 6U );
    LUMEN_CHECK( Triangle.IndexFormat == RHI::ERHIIndexFormat::Index32 );
    LUMEN_CHECK( Quad.IndexFormat == RHI::ERHIIndexFormat::Index32 );

    const RHI::FRHIBufferDesc &TriangleVertexDesc = Triangle.VertexBuffer->GetDesc();
    const RHI::FRHIBufferDesc &TriangleIndexDesc  = Triangle.IndexBuffer->GetDesc();
    LUMEN_CHECK_EQ( TriangleVertexDesc.Size, static_cast<UInt64>( 3ULL * sizeof( Maths::FVec3f ) ) );
    LUMEN_CHECK_EQ( TriangleIndexDesc.Size, static_cast<UInt64>( 3ULL * sizeof( UInt32 ) ) );
    LUMEN_CHECK_TRUE( HasAllUsage( TriangleVertexDesc.Usage, RHI::ERHIResourceUsage::VertexBuffer |
                                                                 RHI::ERHIResourceUsage::CopySource |
                                                                 RHI::ERHIResourceUsage::CopyDestination ) );
    LUMEN_CHECK_TRUE( HasAllUsage( TriangleIndexDesc.Usage, RHI::ERHIResourceUsage::IndexBuffer |
                                                                RHI::ERHIResourceUsage::CopySource |
                                                                RHI::ERHIResourceUsage::CopyDestination ) );
    LUMEN_CHECK( TriangleVertexDesc.MemoryClass == RHI::ERHIMemoryClass::DeviceLocal );
    LUMEN_CHECK( TriangleIndexDesc.MemoryClass == RHI::ERHIMemoryClass::DeviceLocal );
    LUMEN_CHECK_EQ( Quad.VertexBuffer->GetDesc().Size, static_cast<UInt64>( 4ULL * sizeof( Maths::FVec3f ) ) );
    LUMEN_CHECK_EQ( Quad.IndexBuffer->GetDesc().Size, static_cast<UInt64>( 6ULL * sizeof( UInt32 ) ) );

    LUMEN_CHECK( Registry.Find( FRenderAssetId( 1ULL ) ) == &Triangle );
    LUMEN_CHECK( Registry.Find( FRenderAssetId( 2ULL ) ) == &Quad );
    LUMEN_CHECK( Registry.Find( FRenderAssetId( 99ULL ) ) == nullptr );
}

LUMEN_TEST( FRenderAssetRegistry, RejectsDuplicateAndMalformedDescriptors )
{
    const auto Device = MakeTestDevice();
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }
    const auto Queue = MakeTestQueue( **Device );
    LUMEN_CHECK_HAS_VALUE( Queue );
    if ( not Queue )
    {
        return;
    }

    FRenderAssetRegistry Registry( 8ULL );
    const TArray<Maths::FVec3f, 3> Positions = MakeTrianglePositions();
    const TArray<UInt32, 3> Indices{ 0U, 1U, 2U };
    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 5ULL, Positions, Indices ) ) );
    const FRenderGeometryAsset *Original = Registry.Find( FRenderAssetId( 5ULL ) );

    const TArray<Maths::FVec3f, 4> QuadPositions = MakeQuadPositions();
    const TArray<UInt32, 6> QuadIndices{ 0U, 1U, 2U, 0U, 2U, 3U };
    const auto Duplicate = Registry.Register( **Device, **Queue, MakeDesc( 5ULL, QuadPositions, QuadIndices ) );
    LUMEN_CHECK_HAS_ERROR( Duplicate );
    LUMEN_CHECK( Duplicate.Error() == ERenderAssetError::InvalidDesc );
    LUMEN_CHECK_EQ( Registry.GetSize(), 1ULL );
    LUMEN_CHECK( Registry.Find( FRenderAssetId( 5ULL ) ) == Original );
    LUMEN_CHECK_EQ( Original->VertexCount, 3U );

    const TArray<Maths::FVec3f, 2> TooFewPositions{ Maths::FVec3f{ 0.0F }, Maths::FVec3f{ 1.0F } };
    const TArray<UInt32, 3> TriangleIndices{ 0U, 1U, 2U };
    LUMEN_CHECK( Registry.Register( **Device, **Queue, MakeDesc( 0ULL, Positions, TriangleIndices ) ).Error() ==
                 ERenderAssetError::InvalidDesc );
    LUMEN_CHECK( Registry.Register( **Device, **Queue, MakeDesc( 6ULL, TooFewPositions, TriangleIndices ) ).Error() ==
                 ERenderAssetError::InvalidDesc );

    const TArray<UInt32, 2> TooFewIndices{ 0U, 1U };
    LUMEN_CHECK( Registry.Register( **Device, **Queue, MakeDesc( 7ULL, Positions, TooFewIndices ) ).Error() ==
                 ERenderAssetError::InvalidDesc );

    const TArray<UInt32, 4> NotMultipleOfThree{ 0U, 1U, 2U, 0U };
    LUMEN_CHECK( Registry.Register( **Device, **Queue, MakeDesc( 8ULL, QuadPositions, NotMultipleOfThree ) ).Error() ==
                 ERenderAssetError::InvalidDesc );

    const TArray<UInt32, 3> OutOfRangeIndices{ 0U, 1U, 3U };
    LUMEN_CHECK( Registry.Register( **Device, **Queue, MakeDesc( 9ULL, Positions, OutOfRangeIndices ) ).Error() ==
                 ERenderAssetError::InvalidDesc );
    LUMEN_CHECK_EQ( Registry.GetSize(), 1ULL );
    LUMEN_CHECK( Registry.Find( FRenderAssetId( 9ULL ) ) == nullptr );
}

LUMEN_TEST( FRenderAssetRegistry, CapacityOverflowAndZeroCapacityRejectBeforeCreation )
{
    const auto Device = MakeTestDevice();
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }
    const auto Queue = MakeTestQueue( **Device );
    LUMEN_CHECK_HAS_VALUE( Queue );
    if ( not Queue )
    {
        return;
    }

    const TArray<Maths::FVec3f, 3> Positions = MakeTrianglePositions();
    const TArray<UInt32, 3> Indices{ 0U, 1U, 2U };
    FRenderAssetRegistry Full( 1ULL );
    LUMEN_CHECK_HAS_VALUE( Full.Register( **Device, **Queue, MakeDesc( 1ULL, Positions, Indices ) ) );
    const auto Overflow = Full.Register( **Device, **Queue, MakeDesc( 2ULL, Positions, Indices ) );
    LUMEN_CHECK_HAS_ERROR( Overflow );
    LUMEN_CHECK( Overflow.Error() == ERenderAssetError::CapacityExceeded );
    LUMEN_CHECK_EQ( Full.GetSize(), 1ULL );
    LUMEN_CHECK( Full.Find( FRenderAssetId( 2ULL ) ) == nullptr );

    FRenderAssetRegistry Empty( 0ULL );
    const auto Rejected = Empty.Register( **Device, **Queue, MakeDesc( 1ULL, Positions, Indices ) );
    LUMEN_CHECK_HAS_ERROR( Rejected );
    LUMEN_CHECK( Rejected.Error() == ERenderAssetError::CapacityExceeded );
    LUMEN_CHECK_EQ( Empty.GetSize(), 0ULL );
    LUMEN_CHECK_EQ( Empty.GetCapacity(), 0ULL );
}

LUMEN_TEST( FRenderAssetRegistry, GetAssetsUsesCanonicalIdOrder )
{
    const auto Device = MakeTestDevice();
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }
    const auto Queue = MakeTestQueue( **Device );
    LUMEN_CHECK_HAS_VALUE( Queue );
    if ( not Queue )
    {
        return;
    }

    const TArray<Maths::FVec3f, 3> Positions = MakeTrianglePositions();
    const TArray<UInt32, 3> Indices{ 0U, 1U, 2U };
    FRenderAssetRegistry Registry( 3ULL );
    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 20ULL, Positions, Indices ) ) );
    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 5ULL, Positions, Indices ) ) );
    LUMEN_CHECK_HAS_VALUE( Registry.Register( **Device, **Queue, MakeDesc( 12ULL, Positions, Indices ) ) );

    LUMEN_CHECK_EQ( Registry.GetSize(), 3ULL );
    LUMEN_CHECK_EQ( Registry.GetAssets()[0].Id, FRenderAssetId( 5ULL ) );
    LUMEN_CHECK_EQ( Registry.GetAssets()[1].Id, FRenderAssetId( 12ULL ) );
    LUMEN_CHECK_EQ( Registry.GetAssets()[2].Id, FRenderAssetId( 20ULL ) );
}

} // namespace Lumen
