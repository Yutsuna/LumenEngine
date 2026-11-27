/**
 * @file SceneExtractor.cpp
 * @brief Implementation of complete rendering snapshot extraction.
 */

#include <Rendering/SceneExtractor.hpp>

#include <Rendering/RenderSnapshotStream.hpp>

#include <Algo/ForEach.hpp>
#include <Algo/Sort.hpp>
#include <Core/CoreDefinitions.hpp>
#include <Templates/Unexpected.hpp>

namespace
{

[[nodiscard]] Lumen::FRenderSnapshotError MakeError ( const Lumen::ERenderSnapshotErrorCode InCode,
                                                      const Lumen::ERenderSnapshotRecordType InRecordType,
                                                      const Lumen::USize InRecordIndex   = 0ULL,
                                                      const Lumen::USize InRequiredCount = 0ULL,
                                                      const Lumen::USize InCapacity      = 0ULL ) noexcept
{
    return Lumen::FRenderSnapshotError{
        .Code          = InCode,
        .RecordType    = InRecordType,
        .RecordIndex   = InRecordIndex,
        .RequiredCount = InRequiredCount,
        .Capacity      = InCapacity,
    };
}

[[nodiscard]] Lumen::TExpected<void, Lumen::FRenderSnapshotError>
ValidateCapacity ( const Lumen::ERenderSnapshotRecordType InRecordType,
                   const Lumen::USize InRequiredCount,
                   const Lumen::USize InCapacity ) noexcept
{
    if ( InRequiredCount > InCapacity ) [[unlikely]]
    {
        return Lumen::MakeUnexpected(
            MakeError( Lumen::ERenderSnapshotErrorCode::CapacityExceeded, InRecordType, 0ULL, InRequiredCount, InCapacity ) );
    }

    return {};
}

[[nodiscard]] Lumen::TExpected<void, Lumen::FRenderSnapshotError>
ValidateInput ( const Lumen::FSceneExtractionInput &InInput, const Lumen::FRenderSnapshotCapacities &InCapacities ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( ValidateCapacity( Lumen::ERenderSnapshotRecordType::Transform, InInput.Transforms.size(),
                                              InCapacities.TransformCapacity ) );
    LUMEN_TRY_EXPECT_ERROR(
        ValidateCapacity( Lumen::ERenderSnapshotRecordType::Mesh, InInput.Meshes.size(), InCapacities.MeshCapacity ) );
    LUMEN_TRY_EXPECT_ERROR(
        ValidateCapacity( Lumen::ERenderSnapshotRecordType::Camera, InInput.Cameras.size(), InCapacities.CameraCapacity ) );
    LUMEN_TRY_EXPECT_ERROR(
        ValidateCapacity( Lumen::ERenderSnapshotRecordType::Light, InInput.Lights.size(), InCapacities.LightCapacity ) );

    for ( Lumen::USize MeshIndex = 0ULL; MeshIndex < InInput.Meshes.size(); ++MeshIndex )
    {
        const Lumen::FRenderMeshRecord &Mesh = InInput.Meshes[MeshIndex];
        if ( Mesh.TransformIndex >= InInput.Transforms.size() ) [[unlikely]]
        {
            return Lumen::MakeUnexpected( MakeError( Lumen::ERenderSnapshotErrorCode::InvalidTransformIndex,
                                                     Lumen::ERenderSnapshotRecordType::Mesh, MeshIndex, 0ULL,
                                                     InInput.Transforms.size() ) );
        }

        if ( InInput.Transforms[Mesh.TransformIndex].ProxyId != Mesh.ProxyId ) [[unlikely]]
        {
            return Lumen::MakeUnexpected( MakeError( Lumen::ERenderSnapshotErrorCode::TransformProxyMismatch,
                                                     Lumen::ERenderSnapshotRecordType::Mesh, MeshIndex, 0ULL,
                                                     InInput.Transforms.size() ) );
        }
    }

    return {};
}

template <typename RecordType>
void CopyRecords ( Lumen::TVector<RecordType> &OutRecords, const Lumen::TSpan<const RecordType> InRecords ) noexcept
{
    Lumen::Algo::ForEach( InRecords, [&OutRecords] ( const RecordType &InRecord ) noexcept
    {
        OutRecords.emplace_back( InRecord );
    } );
}

template <typename RecordType> void SortByProxyId ( Lumen::TVector<RecordType> &InOutRecords ) noexcept
{
    Lumen::Algo::Sort( InOutRecords, [] ( const RecordType &InLeft, const RecordType &InRight ) noexcept -> Lumen::Bool
    {
        return InLeft.ProxyId < InRight.ProxyId;
    } );
}

[[nodiscard]] Lumen::USize FindTransformIndex ( const Lumen::TVector<Lumen::FRenderTransformRecord> &InTransforms,
                                                const Lumen::FRenderProxyId InProxyId ) noexcept
{
    Lumen::USize Lower = 0ULL;
    Lumen::USize Upper = InTransforms.size();

    while ( Lower < Upper )
    {
        const Lumen::USize Middle = Lower + ( ( Upper - Lower ) / 2ULL );
        if ( InTransforms[Middle].ProxyId < InProxyId )
        {
            Lower = Middle + 1ULL;
        }
        else
        {
            Upper = Middle;
        }
    }

    LUMEN_ASSERT( Lower < InTransforms.size() && InTransforms[Lower].ProxyId == InProxyId );
    return Lower;
}

} // namespace

Lumen::FSceneExtractor::FSceneExtractor ( FRenderSnapshotStream &InStream ) noexcept : Stream( &InStream )
{
    /** Empty */
}

Lumen::TExpected<Lumen::FRenderSnapshotHeader, Lumen::FRenderSnapshotError>
Lumen::FSceneExtractor::Publish ( const FSceneExtractionInput &InInput ) noexcept
{
    LUMEN_ASSERT( Stream != nullptr );
    LUMEN_TRY_EXPECT_ERROR( ValidateInput( InInput, Stream->Capacities ) );

    const FRenderSnapshotHeader Header{
        .WorldId          = InInput.WorldId,
        .FrameIndex       = InInput.FrameIndex,
        .SnapshotSequence = NextSnapshotSequence,
        .TransformCount   = InInput.Transforms.size(),
        .MeshCount        = InInput.Meshes.size(),
        .CameraCount      = InInput.Cameras.size(),
        .LightCount       = InInput.Lights.size(),
        .Capacities       = Stream->Capacities,
    };

    Stream->SnapshotBuffer.WriteBuffer( [&InInput, Header] ( FRenderSnapshotStorage &OutStorage ) noexcept
    {
        OutStorage.Reset();
        OutStorage.Header = Header;

        CopyRecords( OutStorage.Transforms, InInput.Transforms );
        SortByProxyId( OutStorage.Transforms );

        Algo::ForEach( InInput.Meshes, [&OutStorage] ( const FRenderMeshRecord &InMesh ) noexcept
        {
            FRenderMeshRecord Mesh = InMesh;
            Mesh.TransformIndex    = FindTransformIndex( OutStorage.Transforms, Mesh.ProxyId );
            OutStorage.Meshes.emplace_back( Mesh );
        } );
        SortByProxyId( OutStorage.Meshes );

        CopyRecords( OutStorage.Cameras, InInput.Cameras );
        SortByProxyId( OutStorage.Cameras );

        CopyRecords( OutStorage.Lights, InInput.Lights );
        SortByProxyId( OutStorage.Lights );
    } );

    Stream->LastPublishedSequence.store( NextSnapshotSequence, std::memory_order_release );
    ++NextSnapshotSequence;

    return Header;
}
