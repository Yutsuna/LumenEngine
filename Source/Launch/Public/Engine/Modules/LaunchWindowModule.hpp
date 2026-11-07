/**
 * @file LaunchWindowModule.hpp
 * @brief Launch the main window. Requires the Application module.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericWindow;

namespace Launch
{

    /**
     * @class FLaunchWindowModule
     * @brief Owns the main OS window. Dies before the Application module.
     */
    class FLaunchWindowModule final : public IEngineModule
    {
    public:

        explicit FLaunchWindowModule ( TSharedPtr<FGenericWindow> InWindow ) noexcept;
        ~FLaunchWindowModule () noexcept override = default;

    public:

        [[nodiscard]] FStringView GetName () const noexcept override;

    private:

        TSharedPtr<FGenericWindow> Window;
    };

    /** @brief Factory registered in GetEngineModules(). Headless yields a no-op module. */
    [[nodiscard]] TLaunchResult<TUniquePtr<IEngineModule>> MakeWindowModule ( const FLaunchEngineConfig &InConfig ) noexcept;

} // namespace Launch

} // namespace Lumen
