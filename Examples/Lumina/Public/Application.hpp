/**
 * @file Application.hpp
 * @brief Composition root for Lumina's actor-driven path tracer.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchRendererModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

#include <Templates/Optional.hpp>
#include <World/WorldIngressJournal.hpp>

namespace Lumen
{

class ACameraActor;
class FGenericWindow;
class FWorld;

/** @class FLuminaApplication @brief Owns the camera world and its scoped renderer session. */
class FLuminaApplication final : public Launch::IEngineModule
{
public:

    using FRequires         = Launch::TModuleList<Launch::FLaunchRendererModule>;
    using FOptionalRequires = Launch::TModuleList<Launch::FLaunchApplicationModule, Launch::FLaunchWindowModule>;

    static constexpr Launch::ETickPhase TickPhase = Launch::ETickPhase::PreRender;

public:

    FLuminaApplication () noexcept;
    ~FLuminaApplication () noexcept override;

    [[nodiscard]] static Launch::TLaunchResult<TUniquePtr<FLuminaApplication>>
    Create ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept;

public:

    [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override;
    void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept override;

private:

    [[nodiscard]] Launch::TLaunchResult<void> Initialize ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept;
    void UpdateWindowTitle ( UInt64 InSampleCount ) noexcept;

private:

    FWorldIngressJournal IngressJournal;
    TUniquePtr<FWorld> World;
    TOptional<Launch::FScopedRenderSession> Session;
    Launch::FScopedMessageHandler MessageHandlerGuard;
    TSharedPtr<FGenericWindow> Window;
    ACameraActor *CameraActor   = nullptr;
    ActorID Camera              = InvalidActorID;
    UInt64 LastTitleSampleCount = 0ULL;
    Bool bHeadless              = false;
};

} // namespace Lumen
