/**
 * @file FileWatcher.cpp
 * @brief Implementation of the FileWatcher mechanism.
 */

#include "Filesystem/FileWatcher.hpp"
#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include "HAL/PlatformTime.hpp"

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
    TUniqueLock<FMutex> Lock( WatcherMutex );

    if ( WorkerThread.joinable() )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    if ( not FDirectory::Exists( InPath ) and not FFile::Exists( InPath ) )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    TargetPath  = InPath;
    WatchEvents = InEvents;
    Callback    = InCallback;

    if ( FDirectory::Exists( TargetPath ) )
    {
        auto Files = FDirectory::GetFiles( TargetPath, true );
        if ( Files )
        {
            for ( const auto &File : Files.value() )
            {
                LastKnownModifiedTimes[File.Path] = File.LastModified;
            }
        }
    }
    else
    {
        auto FileInfo = FFile::GetInfo( TargetPath );
        if ( FileInfo )
        {
            LastKnownModifiedTimes[FileInfo.value().Path] = FileInfo.value().LastModified;
        }
    }

    WorkerThread = std::jthread( [this] ( std::stop_token InToken ) { PollLoop( InToken ); } );

    return {};
}

void LumenEngine::Filesystem::FFileWatcher::Stop () noexcept
{
    TUniqueLock<FMutex> Lock( WatcherMutex );
    if ( WorkerThread.joinable() )
    {
        WorkerThread.request_stop();
        WorkerThread.join();
    }
    LastKnownModifiedTimes.clear();
}

void LumenEngine::Filesystem::FFileWatcher::PollLoop ( std::stop_token InToken ) noexcept
{
    while ( not InToken.stop_requested() )
    {
        HAL::FPlatformTime::Sleep( 0.25 );

        TUniqueLock<FMutex> Lock( WatcherMutex );

        TMap<FString, Float64> CurrentTimes;

        if ( FDirectory::Exists( TargetPath ) )
        {
            auto Files = FDirectory::GetFiles( TargetPath, true );
            if ( Files )
            {
                for ( const auto &File : Files.value() )
                {
                    CurrentTimes[File.Path] = File.LastModified;
                }
            }
        }
        else if ( FFile::Exists( TargetPath ) )
        {
            auto FileInfo = FFile::GetInfo( TargetPath );
            if ( FileInfo )
            {
                CurrentTimes[FileInfo.value().Path] = FileInfo.value().LastModified;
            }
        }

        for ( const auto &[PathStr, ModTime] : CurrentTimes )
        {
            if ( not LastKnownModifiedTimes.contains( PathStr ) )
            {
                if ( ( WatchEvents & EWatchEvent::Created ) != EWatchEvent::None )
                {
                    Callback( { .Event = EWatchEvent::Created, .Path = PathStr, .OldPath = "", .Timestamp = HAL::FPlatformTime::Seconds() } );
                }
            }
            else if ( LastKnownModifiedTimes[PathStr] < ModTime )
            {
                if ( ( WatchEvents & EWatchEvent::Modified ) != EWatchEvent::None )
                {
                    Callback( { .Event = EWatchEvent::Modified, .Path = PathStr, .OldPath = "", .Timestamp = HAL::FPlatformTime::Seconds() } );
                }
            }
        }

        for ( const auto &[PathStr, ModTime] : LastKnownModifiedTimes )
        {
            if ( not CurrentTimes.contains( PathStr ) )
            {
                if ( ( WatchEvents & EWatchEvent::Deleted ) != EWatchEvent::None )
                {
                    Callback( { .Event = EWatchEvent::Deleted, .Path = PathStr, .OldPath = "", .Timestamp = HAL::FPlatformTime::Seconds() } );
                }
            }
        }

        LastKnownModifiedTimes = std::move( CurrentTimes );
    }
}