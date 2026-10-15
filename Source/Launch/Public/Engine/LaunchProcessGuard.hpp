/// @file LaunchProcessGuard.hpp
/// @brief RAII Owner Launch Services

#pragma once

#include <Core/NonCopyable.hpp>

#include <new>

namespace Lumen
{

namespace Launch
{

    ///
    /// @class FLaunchProcessGuard
    /// @brief RAII Owner Launch Services
    ///
    class FLaunchProcessGuard final : public FNonCopyable
    {

    public:

        FLaunchProcessGuard () noexcept;
        ~FLaunchProcessGuard () noexcept;

    private:

        std::new_handler PreviousNewHandler = nullptr;
    };

} // namespace Launch

} // namespace Lumen