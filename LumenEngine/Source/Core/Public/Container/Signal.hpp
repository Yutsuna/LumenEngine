/**
 * @file Signal.hpp
 * @brief Declaration of the Signal class for event handling and callback management.
 */

#pragma once

#include "Container/Map.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

#include <csignal>

namespace LumenEngine
{

/**
 * @enum ESystemSignal
 * @brief Enumeration of system signals that can be handled by the engine.
 */
namespace ESystemSignal
{

    enum Type : Int32
    {
        None      = 0,
        Interrupt = SIGINT,
        Terminate = SIGTERM,
        Abort     = SIGABRT,
        Hangup    = SIGHUP,
        User1     = SIGUSR1,
        User2     = SIGUSR2,
    };

}; // namespace ESystemSignal

/**
 * @typedef FSignalHandlerSignature
 * @brief Type definition for signal handler functions that can be registered with the FSignal class.
 */
using FSignalHandlerSignature = void ( * )( const ESystemSignal::Type );

/**
 * @class FSignal
 * @brief Static manager to trap and dispatch system signals to registered handlers.
 */
class FSignal
{
public:

    /**
     * @brief Binds a signal handler to a specific system signal.
     * @param InSignal The system signal to bind the handler to.
     * @param inHandler The function pointer to the signal handler to be called when the signal
     */
    static void Bind ( const ESystemSignal::Type InSignal, FSignalHandlerSignature inHandler );

    /**
     * @brief Raises a system signal, invoking any registered handlers for that signal.
     * @param InSignal The system signal to raise.
     */
    static void Raise ( const ESystemSignal::Type InSignal );

    /**
     * @brief Unbinds any signal handler associated with a specific system signal, preventing it from being called when the signal is raised.
     * @param InSignal The system signal to unbind the handler from.
     */
    static void Unbind ( const ESystemSignal::Type InSignal );

private:

    /** @brief Internal handler called by the OS */
    static void HandleOSSignal ( const Int32 InSignal );

private:

    /** Map of registered signal handlers for each system signal */
    static FMap<ESystemSignal::Type, FVector<FSignalHandlerSignature>> SignalListeners;
};

} // namespace LumenEngine
