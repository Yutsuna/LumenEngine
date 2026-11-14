/**
 * @file DirectoryTests.cpp
 * @brief Unit tests for the FDirectory filesystem facade.
 */

#include <Filesystem/Directory.hpp>
#include <Filesystem/File.hpp>
#include <Filesystem/Path.hpp>
#include <Tests/UnitTest.hpp>

namespace
{

Lumen::Filesystem::FPath GetTestPath ( const Lumen::AnsiChar *InName )
{
    return Lumen::Filesystem::FPath( "LumenFilesystemTests" ) / Lumen::FStringView( InName );
}

struct FScopedTestDir
{
    Lumen::Filesystem::FPath Path;

    explicit FScopedTestDir ( const Lumen::AnsiChar *InName, bool bCreate = true ) : Path( GetTestPath( InName ) )
    {
        Lumen::Filesystem::FDirectory::Delete( Path, true );
        if ( bCreate )
        {
            Lumen::Filesystem::FDirectory::CreateDirectories( Path );
        }
    }

    ~FScopedTestDir ()
    {
        Lumen::Filesystem::FDirectory::Delete( Path, true );
    }
};

} // namespace

/** FDirectory::Exists */

LUMEN_TEST( FDirectory_Exists, ReturnsFalseForMissingPath )
{
    const auto TestPath = GetTestPath( "FDirectory_Exists_Missing" );
    Lumen::Filesystem::FDirectory::Delete( TestPath, true );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FDirectory::Exists( TestPath ) );
}

LUMEN_TEST( FDirectory_Exists, ReturnsTrueForExistingPath )
{
    FScopedTestDir Guard( "FDirectory_Exists_Existing" );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FDirectory::Exists( Guard.Path ) );
}

/** FDirectory::Create */

LUMEN_TEST( FDirectory_Create, CreatesNewDirectory )
{
    FScopedTestDir Guard( "FDirectory_Create_New", false );
    const auto Result = Lumen::Filesystem::FDirectory::Create( Guard.Path );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FDirectory::Exists( Guard.Path ) );
}

LUMEN_TEST( FDirectory_Create, FailsWhenDirectoryAlreadyExists )
{
    FScopedTestDir Guard( "FDirectory_Create_Existing", true );
    const auto Result = Lumen::Filesystem::FDirectory::Create( Guard.Path );
    LUMEN_CHECK_HAS_ERROR( Result );
}

/** FDirectory::CreateDirectories */

LUMEN_TEST( FDirectory_CreateDirectories, CreatesNestedDirectories )
{
    FScopedTestDir Guard( "FDirectory_CreateDirectories_Nested", false );
    const auto NestedPath = Guard.Path / "a" / "b" / "c";
    const auto Result     = Lumen::Filesystem::FDirectory::CreateDirectories( NestedPath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FDirectory::Exists( NestedPath ) );
}

LUMEN_TEST( FDirectory_CreateDirectories, SucceedsWhenDirectoryAlreadyExists )
{
    FScopedTestDir Guard( "FDirectory_CreateDirectories_Existing", true );
    const auto Result = Lumen::Filesystem::FDirectory::CreateDirectories( Guard.Path );
    LUMEN_CHECK_HAS_VALUE( Result );
}

/** FDirectory::Delete */

LUMEN_TEST( FDirectory_Delete, DeletesEmptyDirectory )
{
    FScopedTestDir Guard( "FDirectory_Delete_Empty", true );
    const auto Result = Lumen::Filesystem::FDirectory::Delete( Guard.Path, false );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FDirectory::Exists( Guard.Path ) );
}

LUMEN_TEST( FDirectory_Delete, FailsForMissingDirectory )
{
    const auto TestPath = GetTestPath( "FDirectory_Delete_Missing" );
    Lumen::Filesystem::FDirectory::Delete( TestPath, true );
    const auto Result = Lumen::Filesystem::FDirectory::Delete( TestPath, false );
    LUMEN_CHECK_HAS_ERROR( Result );
}

LUMEN_TEST( FDirectory_Delete, FailsForNonEmptyDirectoryWithoutRecursive )
{
    FScopedTestDir Guard( "FDirectory_Delete_NonEmpty", true );
    const auto FilePath = Guard.Path / "file.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "data" );
    const auto Result = Lumen::Filesystem::FDirectory::Delete( Guard.Path, false );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FDirectory::Exists( Guard.Path ) );
}

LUMEN_TEST( FDirectory_Delete, DeletesNonEmptyDirectoryWithRecursive )
{
    FScopedTestDir Guard( "FDirectory_Delete_Recursive", true );
    const auto FilePath = Guard.Path / "file.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "data" );
    const auto Result = Lumen::Filesystem::FDirectory::Delete( Guard.Path, true );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FDirectory::Exists( Guard.Path ) );
}

/** FDirectory::ForEachEntry */

LUMEN_TEST( FDirectory_ForEachEntry, IteratesImmediateEntriesNonRecursive )
{
    FScopedTestDir Guard( "FDirectory_ForEachEntry_NonRecursive", true );
    const auto File1  = Guard.Path / "file1.txt";
    const auto File2  = Guard.Path / "file2.txt";
    const auto SubDir = Guard.Path / "subdir";
    Lumen::Filesystem::FFile::WriteAllText( File1, "a" );
    Lumen::Filesystem::FFile::WriteAllText( File2, "b" );
    Lumen::Filesystem::FDirectory::Create( SubDir );

    Lumen::USize EntryCount = 0;
    Lumen::Bool bSawFile1   = false;
    Lumen::Bool bSawFile2   = false;
    Lumen::Bool bSawSubDir  = false;

    const auto Result = Lumen::Filesystem::FDirectory::ForEachEntry(
        Guard.Path, false, [&] ( const Lumen::Filesystem::FPath &EntryPath, Lumen::Bool bIsDirectory ) -> Lumen::Bool
    {
        ++EntryCount;
        const auto Filename = EntryPath.GetFilename();
        if ( Filename == Lumen::FStringView( "file1.txt" ) )
        {
            bSawFile1 = true;
            LUMEN_CHECK_FALSE( bIsDirectory );
        }
        else if ( Filename == Lumen::FStringView( "file2.txt" ) )
        {
            bSawFile2 = true;
            LUMEN_CHECK_FALSE( bIsDirectory );
        }
        else if ( Filename == Lumen::FStringView( "subdir" ) )
        {
            bSawSubDir = true;
            LUMEN_CHECK_TRUE( bIsDirectory );
        }
        return true;
    } );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( EntryCount, 3U );
    LUMEN_CHECK_TRUE( bSawFile1 );
    LUMEN_CHECK_TRUE( bSawFile2 );
    LUMEN_CHECK_TRUE( bSawSubDir );
}

LUMEN_TEST( FDirectory_ForEachEntry, IteratesAllEntriesRecursive )
{
    FScopedTestDir Guard( "FDirectory_ForEachEntry_Recursive", true );
    const auto File1  = Guard.Path / "file1.txt";
    const auto File2  = Guard.Path / "file2.txt";
    const auto SubDir = Guard.Path / "subdir";
    const auto File3  = SubDir / "file3.txt";
    Lumen::Filesystem::FDirectory::Create( SubDir );
    Lumen::Filesystem::FFile::WriteAllText( File1, "a" );
    Lumen::Filesystem::FFile::WriteAllText( File2, "b" );
    Lumen::Filesystem::FFile::WriteAllText( File3, "c" );

    Lumen::USize EntryCount = 0;
    Lumen::Bool bSawFile3   = false;

    const auto Result = Lumen::Filesystem::FDirectory::ForEachEntry(
        Guard.Path, true, [&] ( const Lumen::Filesystem::FPath &EntryPath, Lumen::Bool bIsDirectory ) -> Lumen::Bool
    {
        ++EntryCount;
        if ( EntryPath.GetFilename() == Lumen::FStringView( "file3.txt" ) )
        {
            bSawFile3 = true;
            LUMEN_CHECK_FALSE( bIsDirectory );
        }
        return true;
    } );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( EntryCount, 4U );
    LUMEN_CHECK_TRUE( bSawFile3 );
}

LUMEN_TEST( FDirectory_ForEachEntry, StopsEarlyWhenCallbackReturnsFalse )
{
    FScopedTestDir Guard( "FDirectory_ForEachEntry_EarlyStop", true );
    Lumen::Filesystem::FFile::WriteAllText( Guard.Path / "file1.txt", "a" );
    Lumen::Filesystem::FFile::WriteAllText( Guard.Path / "file2.txt", "b" );

    Lumen::USize CallCount = 0;
    const auto Result      = Lumen::Filesystem::FDirectory::ForEachEntry(
        Guard.Path, false, [&] ( const Lumen::Filesystem::FPath &, Lumen::Bool ) -> Lumen::Bool
    {
        ++CallCount;
        return false;
    } );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( CallCount, 1U );
}

LUMEN_TEST( FDirectory_ForEachEntry, WorksWithVoidCallback )
{
    FScopedTestDir Guard( "FDirectory_ForEachEntry_Void", true );
    Lumen::Filesystem::FFile::WriteAllText( Guard.Path / "file1.txt", "a" );
    Lumen::Filesystem::FFile::WriteAllText( Guard.Path / "file2.txt", "b" );

    Lumen::USize CallCount = 0;
    const auto Result =
        Lumen::Filesystem::FDirectory::ForEachEntry( Guard.Path, false, [&] ( const Lumen::Filesystem::FPath &, Lumen::Bool )
    {
        ++CallCount;
    } );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( CallCount, 2U );
}

LUMEN_TEST( FDirectory_ForEachEntry, FailsForMissingDirectory )
{
    const auto TestPath = GetTestPath( "FDirectory_ForEachEntry_Missing" );
    Lumen::Filesystem::FDirectory::Delete( TestPath, true );
    const auto Result = Lumen::Filesystem::FDirectory::ForEachEntry(
        TestPath, false, [] ( const Lumen::Filesystem::FPath &, Lumen::Bool ) -> Lumen::Bool
    {
        return true;
    } );
    LUMEN_CHECK_HAS_ERROR( Result );
}
