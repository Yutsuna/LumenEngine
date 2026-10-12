///
/// @file Core/Private/Filesystem/Directory.cpp
/// @brief FDirectory helper implementations.
///

#include <Filesystem/Directory.hpp>
#include <Filesystem/IFileSystem.hpp>

namespace Lumen
{

namespace Filesystem
{

    IFileSystem &GetDefaultFileSystem () noexcept;

    Bool FDirectory::Exists ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().DirectoryExists( InPath );
    }

    Bool FDirectory::Create ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().CreateDirectory( InPath );
    }

    Bool FDirectory::CreateDirectories ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().CreateDirectories( InPath );
    }

    Bool FDirectory::Delete ( const FPath &InPath, Bool bRecursive ) noexcept
    {
        return GetDefaultFileSystem().DeleteDirectory( InPath, bRecursive );
    }

} // namespace Filesystem

} // namespace Lumen
