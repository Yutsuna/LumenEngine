/**
 * @file GameApplicationType.hpp
 * @brief Declaration of the entry points of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

class LUMEN_ENGINE_API IGameApplication
{
public:

    virtual ~IGameApplication () = default;

    /**
     * @brief Initializes the Game Application API, called once at engine startup
     * @param Argc The number of command-line arguments passed to the application
     * @param Argv An array of C-style strings representing the command-line arguments
     * @return An integer error code, where 0 indicates success and non-zero indicates failure
     */
    virtual Int32 Initialize ( Int32 Argc, const AnsiChar *Argv[] ) = 0;

    /**
     * @brief Advances the Engine by one tick, called every frame by the main loop
     * @param DeltaTime The time elapsed since the last tick, in seconds
     */
    virtual void Tick ( Float64 DeltaTime ) = 0;

    /**
     * @brief Shuts down the Game Application API, called once at engine shutdown
     * @note This function is guaranteed to be called before core systems are terminated
     */
    virtual void Shutdown () = 0;
};

namespace Launch
{

    /**
     * @brief Registers the Game Application instance with the Engine
     * @param InGameApplicationInstance A pointer to the user-defined Game Application instance to be registered with the Engine
     */
    LUMEN_ENGINE_API void RegisterGameApplication ( IGameApplication *InGameApplicationInstance ) noexcept;

    /**
     * @brief Retrieves the singleton instance of the Game Application
     * @return A Reference to the singleton instance of the Game Application
     */
    LUMEN_ENGINE_API IGameApplication &GetGameApplication () noexcept;

} // namespace Launch

} // namespace LumenEngine

#include "Inline/GameApplication.inl"
