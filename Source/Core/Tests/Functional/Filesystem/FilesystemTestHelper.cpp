/**
 * @file FilesystemTestHelper.cpp
 * @brief Implementation of the shared filesystem test fixture.
 */

#include "FilesystemTestHelper.hpp"

#include <Filesystem/Directory.hpp>

/**
 * Public
 */

Lumen::Filesystem::FPath Lumen::Tests::GetTestRoot () noexcept
{
    return Lumen::Filesystem::FPath( "LumenFilesystemTests" );
}

Lumen::Filesystem::FPath Lumen::Tests::GetTestPath ( const Lumen::AnsiChar *InName ) noexcept
{
    return GetTestRoot() / Lumen::FStringView( InName );
}

Lumen::Tests::FScopedTestDir::FScopedTestDir ( const Lumen::AnsiChar *InName, const Lumen::Bool bCreate ) noexcept
    : Path( GetTestPath( InName ) )
{
    /** NOTE: The root is a by-product of CreateDirectories; create it explicitly so each test works alone. */
    Lumen::Filesystem::FDirectory::CreateDirectories( GetTestRoot() );
    Lumen::Filesystem::FDirectory::Delete( Path, true );
    if ( bCreate )
    {
        Lumen::Filesystem::FDirectory::CreateDirectories( Path );
    }
}

Lumen::Tests::FScopedTestDir::~FScopedTestDir () noexcept
{
    Lumen::Filesystem::FDirectory::Delete( Path, true );

    /**
     * NOTE: rmdir only succeeds on an empty directory. The last scoped directory to be destroyed
     * therefore removes the root, while earlier destructors leave it for the next one.
     */
    Lumen::Filesystem::FDirectory::Delete( GetTestRoot(), false );
}
