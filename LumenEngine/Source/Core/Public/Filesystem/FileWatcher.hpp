/**
 * @file FileWatcher.hpp
 * @brief Thread-safe file system watcher.
 */

#pragma once

#include "Container/Expected.hpp"
#include "Container/Map.hpp"

#include "ErrorCodes.hpp"

#include "Filesystem/Path.hpp"
#include "Filesystem/Types.hpp"

#include "HAL/Mutex.hpp"

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include <thread>

namespace LumenEngine
{

namespace Filesystem
{

    using TSnapshot = TMap<FString, Float64>;

    /**
     * @class FFileWatcher
     * @brief A high-performance polling-based file watcher using C++23 std::jthread.
     */
    class LUMEN_ENGINE_API FFileWatcher final : public FNonMovable, public FNonCopyable
    {
    public:

        FFileWatcher () noexcept;
        ~FFileWatcher () noexcept;

    public:

        /**
         * @brief Starts watching a directory or file for the specified events.
         * @param InPath Target path to monitor.
         * @param InEvents Flags indicating which changes to listen for.
         * @param InCallback Invoked when an event is triggered.
         */
        TExpected<void, EErrorCode::Type> Watch ( const FPath &InPath, EWatchEvent InEvents, const FWatchCallback &InCallback );

        /**
         * @brief Stops the current watch thread safely.
         */
        void Stop () noexcept;

    private:

        void PollLoop ( std::stop_token &InToken );

    private:

        FPath TargetPath;
        EWatchEvent WatchEvents = EWatchEvent::None;
        FWatchCallback Callback;

        FMutex WatcherMutex;
        std::jthread WorkerThread;

        TSnapshot LastKnownModifiedTimes;
    };

} // namespace Filesystem

} // namespace LumenEngine