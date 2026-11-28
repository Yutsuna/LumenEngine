/**
 * @file RenderAssetRegistry.hpp
 * @brief Capacity-bounded registry of device-local geometry assets.
 */

#pragma once

#include <RendererExport.hpp>

#include <Frame/RenderAssetTypes.hpp>

#include <Memory/Span.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHIDevice;
    class IRHIQueue;
} // namespace RHI

/**
 * @class FRenderAssetRegistry
 * @brief Creates and stores geometry buffers owned by the renderer.
 * @details Registration is a control-time operation: it stages CPU data through
 *          an RHI upload allocation, records one copy command list and waits for
 *          its completion before recycling transient RHI objects. The asset
 *          array is reserved once in the constructor and stays sorted by id.
 */
class RENDERER_EXPORT FRenderAssetRegistry final
{
public:

    /** @brief Reserves storage for InCapacity assets; zero is a valid capacity. */
    explicit FRenderAssetRegistry ( USize InCapacity ) noexcept;

    FRenderAssetRegistry ( const FRenderAssetRegistry & )           = delete;
    FRenderAssetRegistry &operator=( const FRenderAssetRegistry & ) = delete;
    FRenderAssetRegistry ( FRenderAssetRegistry && )                = default;
    FRenderAssetRegistry &operator=( FRenderAssetRegistry && )      = default;
    ~FRenderAssetRegistry ()                                        = default;

    /**
     * @brief Uploads and registers one geometry asset.
     * @param InDevice Portable device used as the sole factory for RHI objects.
     * @param InQueue Queue that executes the staging copies.
     * @param InDesc Borrowed CPU data, validated before any RHI work starts.
     * @return InvalidDesc for a malformed or duplicate descriptor,
     *         CapacityExceeded when the registry is full, or RHIError when any
     *         device, allocator, command or queue operation fails.
     */
    [[nodiscard]] TExpected<void, ERenderAssetError>
    Register ( RHI::IRHIDevice &InDevice, RHI::IRHIQueue &InQueue, const FRenderGeometryAssetDesc &InDesc ) noexcept;

    /** @return The asset with the given id, or null when it is not registered. */
    [[nodiscard]] const FRenderGeometryAsset *Find ( FRenderAssetId InId ) const noexcept;

    /** @return Registered assets in ascending id order. */
    [[nodiscard]] TSpan<const FRenderGeometryAsset> GetAssets () const noexcept;

    /** @return Number of registered assets. */
    [[nodiscard]] USize GetSize () const noexcept;

    /** @return Reserved asset capacity. */
    [[nodiscard]] USize GetCapacity () const noexcept;

private:

    TVector<FRenderGeometryAsset> Assets;
    USize Capacity = 0ULL;
};

} // namespace Lumen
