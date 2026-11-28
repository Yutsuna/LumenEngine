/**
 * @file Application.hpp
 * @brief Composition root for Lumina's actor-driven OpenGL path tracer.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

#include <World/WorldIngressJournal.hpp>

namespace Lumen
{

class ALuminaCameraActor;
class FGenericWindow;
class FRenderer;
class FWorld;

namespace RHI
{
    class IRHIDevice;
    class IRHIModule;
    class IRHISurfaceBridge;
    class IRHISwapchain;
} // namespace RHI

/** @class FLuminaApplication @brief Owns the camera world, OpenGL device and progressive renderer. */
class FLuminaApplication final : public Launch::IEngineModule
{
public:
    using FOptionalRequires = Launch::TModuleList<Launch::FLaunchApplicationModule, Launch::FLaunchWindowModule>;

    static constexpr Launch::ETickPhase TickPhase = Launch::ETickPhase::PreRender;

public:
    FLuminaApplication () noexcept;
    ~FLuminaApplication () noexcept override;

    [[nodiscard]] static Launch::TLaunchResult<TUniquePtr<FLuminaApplication>>
    Create ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept;

    [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override;
    void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept override;

private:
    [[nodiscard]] Launch::TLaunchResult<void>
    Initialize ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept;
    void UpdateWindowTitle ( UInt64 InSampleCount ) noexcept;

private:
    FWorldIngressJournal IngressJournal;
    TUniquePtr<FWorld> World;
    TSharedPtr<ALuminaCameraActor> CameraActor;
    ActorID Camera = InvalidActorID;
    TSharedPtr<RHI::IRHIModule> RHIModule;
    TSharedPtr<RHI::IRHIDevice> Device;
    TSharedPtr<RHI::IRHISurfaceBridge> SurfaceBridge;
    TSharedPtr<RHI::IRHISwapchain> Swapchain;
    TUniquePtr<FRenderer> Renderer;
    TSharedPtr<FGenericWindow> Window;
    Launch::FScopedMessageHandler MessageHandlerGuard;
    UInt64 LastTitleSampleCount = 0ULL;
    Bool bHeadless              = false;
};

} // namespace Lumen
