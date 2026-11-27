/**
 * @file RenderSnapshotStorage.cpp
 * @brief Implementation of owning rendering snapshot storage.
 */

#include <Rendering/RenderSnapshotStorage.hpp>

void Lumen::FRenderSnapshotStorage::Reserve ( const FRenderSnapshotCapacities &InCapacities ) noexcept
{
    Transforms.reserve( InCapacities.TransformCapacity );
    Meshes.reserve( InCapacities.MeshCapacity );
    Cameras.reserve( InCapacities.CameraCapacity );
    Lights.reserve( InCapacities.LightCapacity );
    Header.Capacities = InCapacities;
}

void Lumen::FRenderSnapshotStorage::Reset () noexcept
{
    Transforms.clear();
    Meshes.clear();
    Cameras.clear();
    Lights.clear();
}

Lumen::FRenderSnapshotView Lumen::FRenderSnapshotStorage::GetView () const noexcept
{
    return FRenderSnapshotView{
        .Header     = Header,
        .Transforms = TSpan<const FRenderTransformRecord>{ Transforms.data(), Transforms.size() },
        .Meshes     = TSpan<const FRenderMeshRecord>{ Meshes.data(), Meshes.size() },
        .Cameras    = TSpan<const FRenderCameraRecord>{ Cameras.data(), Cameras.size() },
        .Lights     = TSpan<const FRenderLightRecord>{ Lights.data(), Lights.size() },
    };
}
