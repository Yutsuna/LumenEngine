/**
 * @file LaunchWindowModule.hpp
 * @brief Launch the main window. Requires the Application module.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericWindow;

namespace Launch
{

    /**
     * @class FLaunchWindowModule
     * @brief Owns the main OS window. Guaranteed to start after and shut down before FLaunchApplicationModule.
     */
    class LAUNCH_EXPORT FLaunchWindowModule final : public IEngineModule
    {
    public:

        using FRequires = TModuleList<FLaunchApplicationModule>;

    public:

        [[nodiscard]] static Bool IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept;

        [[nodiscard]] static TLaunchResult<TUniquePtr<FLaunchWindowModule>>
        Create ( const TModuleContext<FLaunchWindowModule> &InContext ) noexcept;

    public:

        explicit FLaunchWindowModule ( TSharedPtr<FGenericWindow> InWindow ) noexcept;
        ~FLaunchWindowModule () noexcept override = default;

    public:

        [[nodiscard]] const TSharedPtr<FGenericWindow> &GetWindow () const noexcept;

    private:

        TSharedPtr<FGenericWindow> Window;
    };

} // namespace Launch

} // namespace Lumen
