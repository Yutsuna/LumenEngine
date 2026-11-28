/**
 * @file MessageHandler.hpp
 * @brief Translates native application events into journaled camera messages.
 */
#pragma once

#include <Generic/GenericApplicationMessageHandler.hpp>
#include <OrbitCamera.hpp>
#include <World/WorldIngressJournal.hpp>

namespace Lumen
{

class FWorld;

namespace Launch
{
    class FLaunchApplicationModule;
    class FScopedMessageHandler;
} // namespace Launch

/** @class FLuminaMessageHandler @brief Decorates the engine handler with Lumina's orbit camera policy. */
class FLuminaMessageHandler final : public FGenericApplicationMessageHandler
{
public:

    FLuminaMessageHandler ( TSharedPtr<FGenericApplicationMessageHandler> InNextHandler,
                            FWorld &InWorld,
                            FWorldIngressJournal &InJournal,
                            ActorID InCamera ) noexcept;

    void OnRequestExit () noexcept override;
    void OnQuit () noexcept override;
    void OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow ) noexcept override;
    void OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow ) noexcept override;
    void OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow ) noexcept override;
    void OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow ) noexcept override;
    void OnMouseDown ( EMouseButton::Type InMouseButton ) noexcept override;
    void OnMouseUp ( EMouseButton::Type InMouseButton ) noexcept override;
    void OnMouseDoubleClick ( EMouseButton::Type InMouseButton ) noexcept override;
    void OnMouseMove ( Int32 InX, Int32 InY ) noexcept override;
    void OnMouseWheel ( Int32 InDelta ) noexcept override;
    void OnKeyDown ( EKeys::Type InKey, Bool bIsRepeated ) noexcept override;
    void OnKeyUp ( EKeys::Type InKey, Bool bIsRepeated ) noexcept override;

private:

    void SendCameraMessage ( const FMessage &InMessage ) noexcept;
    void PublishCameraPose () noexcept;
    void PublishCameraParams ( Float32 InAspectRatio ) noexcept;
    void ResetCamera () noexcept;

private:

    TSharedPtr<FGenericApplicationMessageHandler> NextHandler;
    FWorld *World                 = nullptr;
    FWorldIngressJournal *Journal = nullptr;
    ActorID Camera                = InvalidActorID;
    Int32 LastX                   = 0;
    Int32 LastY                   = 0;
    Bool bDragging                = false;
    FOrbitCameraState State;
};

[[nodiscard]] Launch::FScopedMessageHandler InstallLuminaMessageHandler ( Launch::FLaunchApplicationModule &InApplicationModule,
                                                                          FWorld &InWorld,
                                                                          FWorldIngressJournal &InJournal,
                                                                          ActorID InCamera ) noexcept;

} // namespace Lumen
