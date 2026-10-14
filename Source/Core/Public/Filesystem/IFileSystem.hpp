/// @file IFileSystem.hpp
/// @brief Interface declaration for Filesystem.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Filesystem/IFileHandle.hpp>
#include <Filesystem/Path.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class IFileSystem
    /// @brief Interface for physical or virtual file system operations.
    ///
    class CORE_EXPORT IFileSystem
    {
    public:

        virtual ~IFileSystem () noexcept = default;

    public:

        [[nodiscard]] virtual TExpected<TUniquePtr<IFileHandle>, EFileError>
        OpenFile ( const FPath &InPath, EFileAccess InAccess, EFileMode InMode ) noexcept = 0;

    public:

        [[nodiscard]] virtual Bool FileExists ( const FPath &InPath ) const noexcept      = 0;
        [[nodiscard]] virtual Bool DirectoryExists ( const FPath &InPath ) const noexcept = 0;

    public:

        virtual TExpected<void, EFileError> DeleteFile ( const FPath &InPath ) noexcept                               = 0;
        virtual TExpected<void, EFileError> DeleteDirectory ( const FPath &InPath, Bool bRecursive = false ) noexcept = 0;

        virtual TExpected<void, EFileError> CreateDirectory ( const FPath &InPath ) noexcept   = 0;
        virtual TExpected<void, EFileError> CreateDirectories ( const FPath &InPath ) noexcept = 0;

        virtual TExpected<void, EFileError>
        CopyFile ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept = 0;
        virtual TExpected<void, EFileError>
        MoveFile ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept = 0;

    public:

        [[nodiscard]] virtual TExpected<FFileStat, EFileError> GetStat ( const FPath &InPath ) const noexcept = 0;
    };

    /// @brief Returns the process-wide default platform filesystem instance.
    [[nodiscard]] CORE_EXPORT IFileSystem &GetDefaultFileSystem () noexcept;

} // namespace Filesystem

} // namespace Lumen