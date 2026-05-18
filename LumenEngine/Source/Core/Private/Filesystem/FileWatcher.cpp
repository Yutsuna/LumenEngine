/**
 * @file FileWatcher.cpp
 * @brief Implementation of the FileWatcher mechanism.
 */

#include "Filesystem/FileWatcher.hpp"
#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"

#include "Container/Vector.hpp"

#include "HAL/PlatformTime.hpp"

namespace
{

/**
 * @struct FDiff
 * @brief Represents the differences between two filesystem snapshots
 */
struct FDiff final
{
    LumenEngine::TVector<LumenEngine::FString> Created;
    LumenEngine::TVector<LumenEngine::FString> Modified;
    LumenEngine::TVector<LumenEngine::FString> Deleted;

    [[nodiscard]] LumenEngine::Bool IsEmpty () const noexcept
    {
        return Created.empty() and Modified.empty() and Deleted.empty();
    }
};

/**
 * @brief Builds a snapshot of the filesystem at a given path
 * @param InPath The path to snapshot
 * @return A snapshot of the filesystem
 */
[[nodiscard]] inline LumenEngine::Filesystem::TSnapshot BuildSnapshot ( const LumenEngine::Filesystem::FPath &InPath ) noexcept
{
    LumenEngine::Filesystem::TSnapshot Snap;

    if ( LumenEngine::Filesystem::FDirectory::Exists( InPath ) )
    {
        if ( const auto Files = LumenEngine::Filesystem::FDirectory::GetFiles( InPath, true ) )
        {
            for ( const auto &File : *Files )
            {
                Snap.emplace( File.Path, File.LastModified );
            }
        }
    }
    else if ( LumenEngine::Filesystem::FFile::Exists( InPath ) )
    {
        if ( const auto Info = LumenEngine::Filesystem::FFile::GetInfo( InPath ) )
        {
            Snap.emplace( Info->Path, Info->LastModified );
        }
    }

    return Snap;
}

/**
 * @brief Computes the difference between two filesystem snapshots
 * @param Previous The previous snapshot
 * @param Current The current snapshot
 * @return The computed difference
 */
[[nodiscard]] FDiff ComputeDiff ( const LumenEngine::Filesystem::TSnapshot &Previous, const LumenEngine::Filesystem::TSnapshot &Current ) noexcept
{
    FDiff Result;

    for ( const auto &[Path, ModTime] : Current )
    {
        if ( auto It = Previous.find( Path ); It == Previous.end() )
        {
            Result.Created.emplace_back( Path );
        }
        else if ( It->second < ModTime )
        {
            Result.Modified.emplace_back( Path );
        }
    }

    for ( const auto &[Path, _] : Previous )
    {
        if ( not Current.contains( Path ) )
        {
            Result.Deleted.emplace_back( Path );
        }
    }

    return Result;
}

/**
 * @brief Dispatches filesystem events to the callback
 * @param InDiff The difference to dispatch
 * @param InMask The event mask to filter events
 * @param InCallback The callback to invoke for each event
 */
void DispatchEvent ( const FDiff &InDiff, LumenEngine::Filesystem::EWatchEvent InMask, const LumenEngine::Filesystem::FWatchCallback &InCallback ) noexcept
{
    const LumenEngine::Float64 Now = LumenEngine::HAL::FPlatformTime::Seconds();

    const auto Fire = [&] ( LumenEngine::Filesystem::EWatchEvent InEvent, std::span<const LumenEngine::FString> InPaths )
    {
        if ( ( InMask & InEvent ) == LumenEngine::Filesystem::EWatchEvent::None )
        {
            return;
        }

        for ( const auto &Path : InPaths )
        {
            InCallback( { .Event = InEvent, .Path = Path, .OldPath = {}, .Timestamp = Now } );
        }
    };

    Fire( LumenEngine::Filesystem::EWatchEvent::Created, InDiff.Created );
    Fire( LumenEngine::Filesystem::EWatchEvent::Modified, InDiff.Modified );
    Fire( LumenEngine::Filesystem::EWatchEvent::Deleted, InDiff.Deleted );
}

} // namespace

/**
 * Ctor & Dtor
 */

LumenEngine::Filesystem::FFileWatcher::FFileWatcher () noexcept = default;

LumenEngine::Filesystem::FFileWatcher::~FFileWatcher () noexcept
{
    Stop();
}

/**
 * Public
 */

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FFileWatcher::Watch ( const FPath &InPath, EWatchEvent InEvents, const FWatchCallback &InCallback )
{
    const TUniqueLock<FMutex> Lock( WatcherMutex );

    if ( WorkerThread.joinable() )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    if ( not FDirectory::Exists( InPath ) and not FFile::Exists( InPath ) )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    TargetPath             = InPath;
    WatchEvents            = InEvents;
    Callback               = InCallback;
    LastKnownModifiedTimes = BuildSnapshot( TargetPath );

    WorkerThread = std::jthread( [this] ( std::stop_token InToken ) { PollLoop( InToken ); } );

    return {};
}

void LumenEngine::Filesystem::FFileWatcher::Stop () noexcept
{
    const TUniqueLock<FMutex> Lock( WatcherMutex );

    if ( WorkerThread.joinable() )
    {
        WorkerThread.request_stop();
        WorkerThread.join();
    }

    LastKnownModifiedTimes.clear();
}

/**
 * Private
 */

void LumenEngine::Filesystem::FFileWatcher::PollLoop ( std::stop_token &InToken )
{
    while ( not InToken.stop_requested() )
    {
        HAL::FPlatformTime::Sleep( 0.25 );
        const TUniqueLock<FMutex> Lock( WatcherMutex );

        const TSnapshot Current = BuildSnapshot( TargetPath );
        const FDiff Diff        = ComputeDiff( LastKnownModifiedTimes, Current );

        if ( not Diff.IsEmpty() )
        {
            DispatchEvent( Diff, WatchEvents, Callback );
        }

        LastKnownModifiedTimes = Current;
    }
}