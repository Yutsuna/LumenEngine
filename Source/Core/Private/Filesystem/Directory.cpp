/// @file Directory.cpp
/// @brief Monadic FDirectory helper implementations.

#include <Filesystem/Directory.hpp>
#include <Filesystem/IFileSystem.hpp>

namespace Lumen
{

namespace Filesystem
{

    Bool FDirectory::Exists ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().DirectoryExists( InPath );
    }

    TExpected<void, EFileError> FDirectory::Create ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().CreateDirectory( InPath );
    }

    TExpected<void, EFileError> FDirectory::CreateDirectories ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().CreateDirectories( InPath );
    }

    TExpected<void, EFileError> FDirectory::Delete ( const FPath &InPath, const Bool bRecursive ) noexcept
    {
        return GetDefaultFileSystem().DeleteDirectory( InPath, bRecursive );
    }

} // namespace Filesystem

} // namespace Lumen