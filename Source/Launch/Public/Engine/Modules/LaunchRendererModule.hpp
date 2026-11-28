/**
 * @file LaunchRendererModule.hpp
 * @brief Owns the renderer and exposes a scoped render session to clients.
 * @details Launch itself contains no RHI or backend code: device, backend selection and
 *          presentation are private to Renderer. The module only maps neutral surfaces and
 *          installs a resize handler for the lifetime of the module.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

#include <Core/NonCopyable.hpp>

#include <Execution/RenderFeature.hpp>
#include <Execution/RenderFrameTypes.hpp>
#include <Execution/RenderSurfaceDesc.hpp>
#include <Execution/Renderer.hpp>
#include <Frame/RenderFrameContracts.hpp>

#include <Memory/UniquePtr.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

class FGenericApplicationMessageHandler;
class FGenericWindow;
class FRenderSnapshotStream;

namespace Launch
{

    class FLaunchRendererModule;

    /**
     * @struct FRenderSessionDesc
     * @brief Backend-neutral inputs for one scoped render session.
     */
    struct FRenderSessionDesc final
    {
        FRenderSnapshotStream *SnapshotStream = nullptr;
        FRenderExtent3D Extent{ 1280U, 720U, 1U };
        TSharedPtr<IRenderFeature> Feature;
        FRenderFrameReadbackDesc Readback;
    };

    /**
     * @class FScopedRenderSession
     * @brief RAII handle that starts a renderer session and shuts it down on destruction.
     */
    class LAUNCH_EXPORT FScopedRenderSession final : public FNonCopyable
    {
    public:

        FScopedRenderSession ( FLaunchRendererModule &InModule, const FRenderSessionDesc &InDesc ) noexcept;
        ~FScopedRenderSession () noexcept;

    public:

        /** @brief True only when the underlying renderer started successfully and is still running. */
        [[nodiscard]] Bool IsActive () const noexcept;

        void RequestRender ( UInt64 InFrameIndex ) noexcept;
        void SwapStatusBuffers () noexcept;
        [[nodiscard]] const FRenderFrameStatus &GetStatus () const noexcept;

    private:

        FLaunchRendererModule *Module = nullptr;
    };

    /**
     * @class FLaunchRendererModule
     * @brief Composition-root owner of the renderer, its surface description and its resize handler.
     */
    class LAUNCH_EXPORT FLaunchRendererModule final : public IEngineModule
    {
    public:

        using FOptionalRequires = TModuleList<FLaunchWindowModule, FLaunchApplicationModule>;

        static constexpr ETickPhase TickPhase = ETickPhase::PreRender;

    public:

        [[nodiscard]] static Bool IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept;

        [[nodiscard]] static TLaunchResult<TUniquePtr<FLaunchRendererModule>>
        Create ( const TModuleContext<FLaunchRendererModule> &InContext ) noexcept;

    public:

        FLaunchRendererModule ( TUniquePtr<FRenderer> InRenderer, FRenderSurfaceDesc InSurface, FString InBackend ) noexcept;
        ~FLaunchRendererModule () noexcept override;

    public:

        /** @brief Installs the resize handler after construction so it never holds a stale renderer pointer. */
        void InstallResizeHandler ( FLaunchApplicationModule &InAppModule ) noexcept;

        [[nodiscard]] EModuleTraits GetTraits () const noexcept override;

        [[nodiscard]] FRenderer &GetRenderer () noexcept;
        [[nodiscard]] const FRenderSurfaceDesc &GetSurfaceDesc () const noexcept;
        [[nodiscard]] const FString &GetDeviceDescription () const noexcept;

        /** @brief Starts the renderer for a session; recreates it after a previous session shut down. */
        [[nodiscard]] TExpected<void, ERendererError> StartSession ( const FRenderSessionDesc &InDesc ) noexcept;

    private:

        friend class FScopedRenderSession;

        TUniquePtr<FRenderer> Renderer;
        FRenderSurfaceDesc Surface;
        FString Backend;
        TOptional<FScopedMessageHandler> ResizeHandler;
        Bool bRendererStopped = false;
    };

} // namespace Launch

} // namespace Lumen
