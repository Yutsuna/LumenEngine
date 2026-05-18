
/**
 * @file FilesystemTypes.hpp
 * @brief This file contains type definitions and enumerations
 */

#pragma once

#include "Container/Function.hpp"
#include "Container/String.hpp"
#include "CoreTypes.hpp"
#include "EnumFlags.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    /**
     * @enum EFileMode
     * @brief Enumeration for file opening modes
     */
    enum class EFileMode : UInt8
    {
        None       = 0,
        Read       = 1 << 0,
        Write      = 1 << 1,
        Append     = 1 << 2,
        Truncate   = 1 << 3,
        Binary     = 1 << 4,
        Exclusive  = 1 << 5,
        ShareRead  = 1 << 6,
        ShareWrite = 1 << 7,

        ReadOnly   = Read | Binary,
        WriteOnly  = Write | Binary | Truncate,
        ReadWrite  = Read | Write | Binary,
        AppendText = Write | Append,
    };
    LUMEN_ENUM_FLAGS( EFileMode );

    /**
     * @enum ESeekOrigin
     * @brief Enumeration for file seek origins
     */
    enum class ESeekOrigin : UInt8
    {
        Begin,
        Current,
        End,
    };

    /**
     * @enum EFileAttributes
     * @brief Enumeration for file attributes
     */
    enum class EFileAttributes : UInt8
    {
        None      = 0,
        ReadOnly  = 1 << 0,
        Hidden    = 1 << 1,
        System    = 1 << 2,
        Directory = 1 << 3,
        Archive   = 1 << 4,
        SymLink   = 1 << 5,
        Temporary = 1 << 6,
    };
    LUMEN_ENUM_FLAGS( EFileAttributes );

    /**
     * @enum EWatchEvent
     * @brief Enumeration for file watch events
     */
    enum class EWatchEvent : UInt8
    {
        None     = 0,
        Created  = 1 << 0,
        Deleted  = 1 << 1,
        Modified = 1 << 2,
        Renamed  = 1 << 3,
        All      = Created | Deleted | Modified | Renamed,
    };
    LUMEN_ENUM_FLAGS( EWatchEvent );

    /**
     * @enum ECopyOptions
     * @brief Enumeration for file copy options
     */
    enum class ECopyOptions : UInt8
    {
        None               = 0,
        SkipExisting       = 1 << 0,
        OverwriteExisting  = 1 << 1,
        UpdateExisting     = 1 << 2,
        Recursive          = 1 << 3,
        CopySymlinks       = 1 << 4,
        SkipSymlinks       = 1 << 5,
        PreserveAttributes = 1 << 6,
    };
    LUMEN_ENUM_FLAGS( ECopyOptions );

    /**
     * @struct FFileInfo
     * @brief Structure for storing file information
     */
    struct FFileInfo
    {
        FString Path;
        FString Name;
        FString Stem;
        FString Extension;

        USize SizeBytes = 0;

        Float64 LastModified = 0.0;
        Float64 LastAccessed = 0.0;
        Float64 CreatedAt    = 0.0;

        EFileAttributes Attributes = EFileAttributes::None;

        [[nodiscard]] Bool IsDirectory () const noexcept;
        [[nodiscard]] Bool IsSymLink () const noexcept;
        [[nodiscard]] Bool IsReadOnly () const noexcept;
    };

    /**
     * @struct FWatchEventData
     * @brief Structure for storing file watch event data
     */
    struct FWatchEventData
    {
        EWatchEvent Event = EWatchEvent::None;
        FString Path;
        FString OldPath;
        Float64 Timestamp = {};
    };

    /**
     * @struct FDiskSpaceInfo
     * @brief Structure for storing disk space information
     */
    struct FDiskSpaceInfo
    {
        USize Capacity  = 0;
        USize Free      = 0;
        USize Available = 0;
    };

    using FWatchCallback    = TFunction<void( const FWatchEventData &WatchEventData )>;
    using FProgressCallback = TFunction<void( USize Done, USize Total )>;
    using FFilterPredicate  = TFunction<Bool( const FFileInfo &FileInfo )>;

} // namespace Filesystem

} // namespace LumenEngine