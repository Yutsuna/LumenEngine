/**
 * @file PlatformSignal.hpp
 * @brief Declaration of the FPlatformSignal class for event handling and callback management.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Templates/Map.hpp>
#include <Templates/Vector.hpp>

#include <csignal>

namespace Lumen
{

namespace HAL
{

    /**
     * @enum ESystemSignal
     * @brief System signals that can be handled by the engine.
     */
    namespace ESystemSignal
    {
        enum Type : UInt8
        {
            None      = 0,
            Interrupt = SIGINT,
            Terminate = SIGTERM,
            Abort     = SIGABRT,
#if defined( _WIN32 )
            Break = SIGBREAK,
#elif defined( __unix__ ) or defined( __APPLE__ )
            Hangup = SIGHUP,
            User1  = SIGUSR1,
            User2  = SIGUSR2,
#else
    #error "Signal.hpp: unsupported platform"
#endif
        };
    } // namespace ESystemSignal

    /**
     * @typedef FSignalHandlerSignature
     * @brief Type definition for signal handler functions that can be registered with the FSignal class.
     */
    using FSignalHandlerSignature = void ( * )( ESystemSignal::Type );

    /**
     * @class FPlatformSignal
     * @brief Static manager to trap system signals and dispatch them to registered handlers.
     */
    class CORE_EXPORT FPlatformSignal
    {
    public:

        using FSignalHandlerVector = TVector<FSignalHandlerSignature>;
        using FSignalHandlerMap    = TMap<ESystemSignal::Type, FSignalHandlerVector>;

    public:

        /**
         * @brief Binds a signal handler to a specific system signal.
         * @param InSignal The system signal to bind the handler to.
         * @param InHandler The function pointer to the signal handler to be called by Dispatch().
         */
        static void Bind ( ESystemSignal::Type InSignal, FSignalHandlerSignature InHandler );

        /**
         * @brief Raises a system signal, then dispatches pending signals to the registered handlers.
         * @param InSignal The system signal to raise.
         */
        static void Raise ( ESystemSignal::Type InSignal );

        /**
         * @brief Runs the handlers of every signal received since the last call. Call it from a normal thread.
         */
        static void Dispatch ();

        /**
         * @brief Unbinds any signal handler associated with a specific system signal.
         * @param InSignal The system signal to unbind the handler from.
         */
        static void Unbind ( ESystemSignal::Type InSignal );

        /**
         * @brief Restores all registered OS handlers to defaults.
         */
        static void Reset ();

    private:

        /** @brief Internal handler called by the OS. Async-signal-safe: only touches PendingSignals. */
        static void HandleOSSignal ( Int32 InSignal );

        /** @brief Internal helper to snapshot the handlers for a given signal, under the lock. */
        static FSignalHandlerVector SnapshotHandlers ( ESystemSignal::Type InSignal );

    private:

        /** @brief Map of registered signal handlers for each system signal (guarded by ListenersMutex) */
        static FSignalHandlerMap SignalListeners;

        /** @brief Protects SignalListeners. Never taken from the OS signal handler (it could interrupt the holder and
         * self-deadlock). */
        static FPlatformRecursiveSharedMutex ListenersMutex;

        /** @brief Bit N is set when signal N was received and not dispatched yet. */
        static TAtomic<UInt64> PendingSignals;
    };

    using FSignal = FPlatformSignal;

} // namespace HAL

} // namespace Lumen
