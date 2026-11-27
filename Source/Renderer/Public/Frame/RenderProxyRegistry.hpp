/**
 * @file RenderProxyRegistry.hpp
 * @brief Capacity-bounded, allocation-free registry of canonical mesh proxies.
 */

#pragma once

#include <RendererExport.hpp>

#include <Frame/RenderProxyTypes.hpp>

#include <Memory/Span.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @class FRenderProxyRegistry
 * @brief Stores one sorted entry per valid mesh proxy in the latest snapshot.
 * @details Both the committed entries and the sorting scratch space are reserved
 *          once in the constructor. A snapshot is validated in full before any
 *          committed state changes, so an overflow or invalid input can never
 *          leave a partially applied proxy set behind. Snapshot spans are read
 *          only during Sync and are never retained past the call.
 */
class RENDERER_EXPORT FRenderProxyRegistry final
{
public:

    /** @brief Reserves storage for InCapacity proxies; zero is a valid capacity. */
    explicit FRenderProxyRegistry ( USize InCapacity ) noexcept;

    FRenderProxyRegistry ( const FRenderProxyRegistry & )           = delete;
    FRenderProxyRegistry &operator=( const FRenderProxyRegistry & ) = delete;
    FRenderProxyRegistry ( FRenderProxyRegistry && )                = default;
    FRenderProxyRegistry &operator=( FRenderProxyRegistry && )      = default;
    ~FRenderProxyRegistry ()                                        = default;

    /**
     * @brief Replaces the registry contents with the valid proxies of one snapshot.
     * @param InMeshes Mesh records, in any order.
     * @param InTransforms Transform records indexed by FRenderMeshRecord::TransformIndex.
     * @param InSyncSequence Sequence stamped onto every surviving proxy.
     * @param OutStats Receives the counters only when the snapshot is accepted.
     * @return CapacityExceeded when more distinct valid proxies than Capacity are
     *         present; in that case entries and stats are left untouched.
     */
    [[nodiscard]] TExpected<void, ERenderProxyError> Sync ( TSpan<const FRenderMeshRecord> InMeshes,
                                                            TSpan<const FRenderTransformRecord> InTransforms,
                                                            UInt64 InSyncSequence,
                                                            FRenderProxyStats &OutStats ) noexcept;

    /** @return A pointer to the matching entry, or empty when no proxy matches. */
    [[nodiscard]] TOptional<const FRenderProxyEntry *> Find ( FRenderProxyId InId ) const noexcept;

    /** @return The committed entries in ascending ProxyId order. */
    [[nodiscard]] TSpan<const FRenderProxyEntry> GetEntries () const noexcept;

    /** @return Number of committed proxies. */
    [[nodiscard]] USize GetSize () const noexcept;

    /** @return Reserved proxy capacity. */
    [[nodiscard]] USize GetCapacity () const noexcept;

    /** @brief Discards all committed proxies while retaining reserved storage. */
    void Clear () noexcept;

private:

    TVector<FRenderProxyEntry> Entries;
    /** @brief Sorted, deduplicated staging set; kept sorted to avoid any post-construction allocation. */
    TVector<FRenderProxyEntry> ScratchEntries;
    USize Capacity = 0ULL;
};

} // namespace Lumen
