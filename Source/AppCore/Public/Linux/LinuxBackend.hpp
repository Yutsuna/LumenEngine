/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <AppCoreExport.hpp>
    #include <Core/CoreTypes.hpp>

namespace Lumen
{

struct APPCORE_EXPORT FLinuxBackend
{
    /** @return True if SDL library is initialized */
    static Bool InitializeSDL () noexcept;

    /** @brief Shutdown SDL library */
    static void ShutdownSDL () noexcept;

    /** @brief Pumps the SDL Event Queue, processing all pending events */
    static void PumpSDLMessagesQueue () noexcept;
};

} // namespace Lumen

#endif