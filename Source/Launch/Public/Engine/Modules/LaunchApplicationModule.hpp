/**
 * @file LaunchApplicationModule.hpp
 * @brief Launch the application module.
 */

#pragma once

#include <Core/NonCopyable.hpp>

#include <Engine/LaunchEngineModule.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

class FGenericApplication;

namespace Launch
{

    class FLaunchApplicationModule;

    namespace Internal
    {
        struct FMessageHandlerNode;
    } // namespace Internal

    /**
     * @class FScopedMessageHandler
     * @brief RAII guard that automatically pops an installed message handler on destruction.
     */
    class LAUNCH_EXPORT FScopedMessageHandler final : public FNonCopyable
    {
    public:

        FScopedMessageHandler () noexcept = default;
        FScopedMessageHandler ( FLaunchApplicationModule *InOwner, const Internal::FMessageHandlerNode *InToken ) noexcept;
        FScopedMessageHandler ( FScopedMessageHandler &&Other ) noexcept;
        FScopedMessageHandler &operator=( FScopedMessageHandler &&Other ) noexcept;
        ~FScopedMessageHandler () noexcept;

    public:

        void Reset () noexcept;

    private:

        FLaunchApplicationModule *Owner            = nullptr;
        const Internal::FMessageHandlerNode *Token = nullptr;
    };

    /**
     * @class FLaunchMessageHandler
     * @brief Routes OS-level requests to the engine loop.
     */
    class FLaunchMessageHandler final : public FGenericApplicationMessageHandler
    {
    public:

        void OnRequestExit () noexcept override;
        void OnQuit () noexcept override;
    };

    /**
     * @class FLaunchApplicationModule
     * @brief Launches and owns the platform application instance.
     */
    class LAUNCH_EXPORT FLaunchApplicationModule final : public IEngineModule
    {
    public:

        static constexpr ETickPhase TickPhase = ETickPhase::Input;

    public:

        [[nodiscard]] static Bool IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept;

        [[nodiscard]] static TLaunchResult<TUniquePtr<FLaunchApplicationModule>>
        Create ( const TModuleContext<FLaunchApplicationModule> &InContext ) noexcept;

    public:

        explicit FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept;
        ~FLaunchApplicationModule () noexcept override;

    public:

        [[nodiscard]] EModuleTraits GetTraits () const noexcept override;

        void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept override;

        [[nodiscard]] const TSharedPtr<FGenericApplication> &GetApplication () const noexcept;

        /**
         * @brief Constructs `THandler( NextHandler, InArgs... )`, installs it on the application,
         *        and returns an RAII guard that restores the previous handler upon destruction.
         */
        template <typename THandler, typename... TArgs>
        [[nodiscard]] FScopedMessageHandler PushMessageHandler ( TArgs &&...InArgs ) noexcept;

    private:

        friend class FScopedMessageHandler;

        [[nodiscard]] TSharedPtr<FGenericApplicationMessageHandler> GetCurrentMessageHandler () const noexcept;

        [[nodiscard]] FScopedMessageHandler
        InstallMessageHandler ( const TSharedRef<FGenericApplicationMessageHandler> &InHandler ) noexcept;

        void PopMessageHandler ( const Internal::FMessageHandlerNode *InToken ) noexcept;

    private:

        TSharedPtr<FGenericApplication> Application;
        TSharedRef<FLaunchMessageHandler> MessageHandler;
        TVector<TUniquePtr<Internal::FMessageHandlerNode>> HandlerStack;
    };

} // namespace Launch

} // namespace Lumen

#include "Inline/LaunchApplicationModule.inl"
