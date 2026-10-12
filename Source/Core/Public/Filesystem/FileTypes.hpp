/// @file FileTypes.hpp
/// @brief Contains file-related enumerations and structures.

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>
#include <CoreExport.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @enum EFileAccess
    /// @brief Specifies the access mode for a file.
    ///
    enum class EFileAccess : UInt8
    {
        None      = 0,
        Read      = 1 << 0,
        Write     = 1 << 1,
        ReadWrite = Read | Write
    };

    LUMEN_ENUM_FLAGS( EFileAccess )

    ///
    /// @enum EFileMode
    /// @brief Specifies the handling mode for opening or creating a file.
    ///
    enum class EFileMode : UInt8
    {
        OpenExisting, /// Opens the file if it exists, otherwise fails
        CreateAlways, /// Overwrites or creates a new file
        OpenOrCreate, /// Opens if exists, creates if not
        Append,       /// Opens or creates and sets cursor at the end
    };

    ///
    /// @enum ESeekOrigin
    /// @brief Specifies the origin for seeking within a file.
    ///
    enum class ESeekOrigin : UInt8
    {
        Begin,
        Current,
        End
    };

    ///
    /// @enum EFileError
    /// @brief List of different file errors.
    ///
    enum class EFileError : UInt8
    {
        None = 0,
        NotFound,
        AccessDenied,
        AlreadyExists,
        DiskFull,
        InvalidHandle,
        PathTooLong,
        SharingViolation,
        NotADirectory,
        IsADirectory,
        IoError,
        Unknown
    };

    ///
    /// @struct FFileStat
    /// @brief File stat mapping (size, permissions, timestamps).
    ///
    struct CORE_EXPORT FFileStat
    {
        UInt64 FileSize         = 0;
        UInt64 LastModifiedTime = 0; /// Unix timestamp in milliseconds
        Bool bIsDirectory       = false;
        Bool bIsReadOnly        = false;
        Bool bExists            = false;

        [[nodiscard]] constexpr Bool IsValid () const noexcept
        {
            return bExists;
        }
    };

} // namespace Filesystem

} // namespace Lumen