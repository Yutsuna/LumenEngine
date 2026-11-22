/**
 * @file FileTests.cpp
 * @brief Unit tests for the FFile filesystem facade.
 */

#include <Filesystem/Directory.hpp>
#include <Filesystem/File.hpp>
#include <Filesystem/Path.hpp>
#include <Tests/UnitTest.hpp>

/**
 * Private
 */

namespace
{

Lumen::Filesystem::FPath GetTestPath ( const Lumen::AnsiChar *InName )
{
    return Lumen::Filesystem::FPath( "LumenFilesystemTests" ) / Lumen::FStringView( InName );
}

struct FScopedTestDir
{
    Lumen::Filesystem::FPath Path;

    explicit FScopedTestDir ( const Lumen::AnsiChar *InName, Lumen::Bool bCreate = true ) : Path( GetTestPath( InName ) )
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

/**
 * FFile::Exists
 */

LUMEN_TEST( FFile_Exists, ReturnsFalseForMissingFile )
{
    const FScopedTestDir Guard( "FFile_Exists_Missing", true );
    const auto FilePath = Guard.Path / "missing.txt";
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FFile::Exists( FilePath ) );
}

LUMEN_TEST( FFile_Exists, ReturnsTrueForExistingFile )
{
    const FScopedTestDir Guard( "FFile_Exists_Existing", true );
    const auto FilePath    = Guard.Path / "existing.txt";
    const auto WriteResult = Lumen::Filesystem::FFile::WriteAllText( FilePath, "content" );
    LUMEN_CHECK_HAS_VALUE( WriteResult );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( FilePath ) );
}

/**
 * FFile::WriteAllText / ReadAllText
 */

LUMEN_TEST( FFile_WriteAllText, WritesNewFile )
{
    const FScopedTestDir Guard( "FFile_WriteAllText_New", true );
    const auto FilePath = Guard.Path / "new.txt";
    const auto Result   = Lumen::Filesystem::FFile::WriteAllText( FilePath, "Hello, World!" );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( FilePath ) );

    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( FilePath );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "Hello, World!" ) );
}

LUMEN_TEST( FFile_WriteAllText, OverwritesExistingFile )
{
    const FScopedTestDir Guard( "FFile_WriteAllText_Overwrite", true );
    const auto FilePath = Guard.Path / "overwrite.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "old" );
    const auto Result = Lumen::Filesystem::FFile::WriteAllText( FilePath, "new" );
    LUMEN_CHECK_HAS_VALUE( Result );
    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( FilePath );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "new" ) );
}

LUMEN_TEST( FFile_WriteAllText, FailsWhenParentDirectoryMissing )
{
    const FScopedTestDir Guard( "FFile_WriteAllText_MissingParent", false );
    const auto FilePath = Guard.Path / "file.txt";
    const auto Result   = Lumen::Filesystem::FFile::WriteAllText( FilePath, "data" );
    LUMEN_CHECK_HAS_ERROR( Result );
}

LUMEN_TEST( FFile_WriteAllText, WritesEmptyFile )
{
    const FScopedTestDir Guard( "FFile_WriteAllText_Empty", true );
    const auto FilePath = Guard.Path / "empty.txt";
    const auto Result   = Lumen::Filesystem::FFile::WriteAllText( FilePath, "" );
    LUMEN_CHECK_HAS_VALUE( Result );
    const auto SizeResult = Lumen::Filesystem::FFile::GetSize( FilePath );
    LUMEN_CHECK_HAS_VALUE( SizeResult );
    LUMEN_CHECK_EQ( *SizeResult, 0ULL );
}

LUMEN_TEST( FFile_ReadAllText, ReadsExistingFile )
{
    const FScopedTestDir Guard( "FFile_ReadAllText_Existing", true );
    const auto FilePath = Guard.Path / "read.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "readme" );
    const auto Result = Lumen::Filesystem::FFile::ReadAllText( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, Lumen::FString( "readme" ) );
}

LUMEN_TEST( FFile_ReadAllText, FailsForMissingFile )
{
    const FScopedTestDir Guard( "FFile_ReadAllText_Missing", true );
    const auto FilePath = Guard.Path / "missing.txt";
    const auto Result   = Lumen::Filesystem::FFile::ReadAllText( FilePath );
    LUMEN_CHECK_HAS_ERROR( Result );
}

LUMEN_TEST( FFile_ReadAllText, ReturnsEmptyForEmptyFile )
{
    const FScopedTestDir Guard( "FFile_ReadAllText_Empty", true );
    const auto FilePath = Guard.Path / "empty.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "" );
    const auto Result = Lumen::Filesystem::FFile::ReadAllText( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Result->empty() );
}

/**
 * FFile::WriteAllBytes / ReadAllBytes
 */

LUMEN_TEST( FFile_WriteAllBytes, WritesBytes )
{
    const FScopedTestDir Guard( "FFile_WriteAllBytes_Success", true );
    const auto FilePath      = Guard.Path / "bytes.bin";
    const Lumen::Byte Data[] = { 0x01, 0x02, 0x03, 0x04 };
    const Lumen::TSpan<const Lumen::Byte> Span( Data, 4 );
    const auto Result = Lumen::Filesystem::FFile::WriteAllBytes( FilePath, Span );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( FilePath ) );

    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllBytes( FilePath );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    const auto &Bytes = *ReadResult;
    LUMEN_CHECK_EQ( Bytes.size(), 4U );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[0] ), 0x01U );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[1] ), 0x02U );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[2] ), 0x03U );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[3] ), 0x04U );
}

LUMEN_TEST( FFile_WriteAllBytes, WritesEmptyBytes )
{
    const FScopedTestDir Guard( "FFile_WriteAllBytes_Empty", true );
    const auto FilePath                             = Guard.Path / "empty.bin";
    const Lumen::TSpan<const Lumen::Byte> EmptySpan = {};
    const auto Result                               = Lumen::Filesystem::FFile::WriteAllBytes( FilePath, EmptySpan );
    LUMEN_CHECK_HAS_VALUE( Result );
    const auto SizeResult = Lumen::Filesystem::FFile::GetSize( FilePath );
    LUMEN_CHECK_HAS_VALUE( SizeResult );
    LUMEN_CHECK_EQ( *SizeResult, 0ULL );
}

LUMEN_TEST( FFile_ReadAllBytes, ReadsBytes )
{
    const FScopedTestDir Guard( "FFile_ReadAllBytes_Success", true );
    const auto FilePath      = Guard.Path / "bytes.bin";
    const Lumen::Byte Data[] = { 0xAA, 0xBB, 0xCC };
    const Lumen::TSpan<const Lumen::Byte> Span( Data, 3 );
    Lumen::Filesystem::FFile::WriteAllBytes( FilePath, Span );

    const auto Result = Lumen::Filesystem::FFile::ReadAllBytes( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    const auto &Bytes = *Result;
    LUMEN_CHECK_EQ( Bytes.size(), 3U );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[0] ), 0xAAU );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[1] ), 0xBBU );
    LUMEN_CHECK_EQ( static_cast<Lumen::UInt32>( Bytes[2] ), 0xCCU );
}

LUMEN_TEST( FFile_ReadAllBytes, FailsForMissingFile )
{
    const FScopedTestDir Guard( "FFile_ReadAllBytes_Missing", true );
    const auto FilePath = Guard.Path / "missing.bin";
    const auto Result   = Lumen::Filesystem::FFile::ReadAllBytes( FilePath );
    LUMEN_CHECK_HAS_ERROR( Result );
}

LUMEN_TEST( FFile_ReadAllBytes, ReturnsEmptyForEmptyFile )
{
    const FScopedTestDir Guard( "FFile_ReadAllBytes_Empty", true );
    const auto FilePath = Guard.Path / "empty.bin";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "" );
    const auto Result = Lumen::Filesystem::FFile::ReadAllBytes( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->size(), 0U );
}

/**
 * FFile::Delete
 */

LUMEN_TEST( FFile_Delete, DeletesExistingFile )
{
    const FScopedTestDir Guard( "FFile_Delete_Existing", true );
    const auto FilePath = Guard.Path / "delete.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "data" );
    const auto Result = Lumen::Filesystem::FFile::Delete( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FFile::Exists( FilePath ) );
}

LUMEN_TEST( FFile_Delete, FailsForMissingFile )
{
    const FScopedTestDir Guard( "FFile_Delete_Missing", true );
    const auto FilePath = Guard.Path / "missing.txt";
    const auto Result   = Lumen::Filesystem::FFile::Delete( FilePath );
    LUMEN_CHECK_HAS_ERROR( Result );
}

/**
 * FFile::Copy
 */

LUMEN_TEST( FFile_Copy, CopiesFile )
{
    const FScopedTestDir Guard( "FFile_Copy_Success", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "copy me" );
    const auto Result = Lumen::Filesystem::FFile::Copy( Source, Dest );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( Dest ) );
    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( Dest );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "copy me" ) );
}

LUMEN_TEST( FFile_Copy, FailsWhenDestinationExistsAndNoOverwrite )
{
    const FScopedTestDir Guard( "FFile_Copy_NoOverwrite", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "source" );
    Lumen::Filesystem::FFile::WriteAllText( Dest, "dest" );
    const auto Result = Lumen::Filesystem::FFile::Copy( Source, Dest, false );
    LUMEN_CHECK_HAS_ERROR( Result );
}

LUMEN_TEST( FFile_Copy, OverwritesWhenDestinationExistsAndOverwriteTrue )
{
    const FScopedTestDir Guard( "FFile_Copy_Overwrite", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "new content" );
    Lumen::Filesystem::FFile::WriteAllText( Dest, "old content" );
    const auto Result = Lumen::Filesystem::FFile::Copy( Source, Dest, true );
    LUMEN_CHECK_HAS_VALUE( Result );
    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( Dest );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "new content" ) );
}

LUMEN_TEST( FFile_Copy, FailsWhenSourceMissing )
{
    const FScopedTestDir Guard( "FFile_Copy_MissingSource", true );
    const auto Source = Guard.Path / "missing.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    const auto Result = Lumen::Filesystem::FFile::Copy( Source, Dest );
    LUMEN_CHECK_HAS_ERROR( Result );
}

/**
 * FFile::Move
 */

LUMEN_TEST( FFile_Move, MovesFile )
{
    const FScopedTestDir Guard( "FFile_Move_Success", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "move me" );
    const auto Result = Lumen::Filesystem::FFile::Move( Source, Dest );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FFile::Exists( Source ) );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( Dest ) );
    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( Dest );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "move me" ) );
}

LUMEN_TEST( FFile_Move, FailsWhenDestinationExistsAndNoOverwrite )
{
    const FScopedTestDir Guard( "FFile_Move_NoOverwrite", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "source" );
    Lumen::Filesystem::FFile::WriteAllText( Dest, "dest" );
    const auto Result = Lumen::Filesystem::FFile::Move( Source, Dest, false );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( Source ) );
    LUMEN_CHECK_TRUE( Lumen::Filesystem::FFile::Exists( Dest ) );
}

LUMEN_TEST( FFile_Move, OverwritesWhenDestinationExistsAndOverwriteTrue )
{
    const FScopedTestDir Guard( "FFile_Move_Overwrite", true );
    const auto Source = Guard.Path / "source.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    Lumen::Filesystem::FFile::WriteAllText( Source, "new content" );
    Lumen::Filesystem::FFile::WriteAllText( Dest, "old content" );
    const auto Result = Lumen::Filesystem::FFile::Move( Source, Dest, true );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_FALSE( Lumen::Filesystem::FFile::Exists( Source ) );
    const auto ReadResult = Lumen::Filesystem::FFile::ReadAllText( Dest );
    LUMEN_CHECK_HAS_VALUE( ReadResult );
    LUMEN_CHECK_EQ( *ReadResult, Lumen::FString( "new content" ) );
}

LUMEN_TEST( FFile_Move, FailsWhenSourceMissing )
{
    const FScopedTestDir Guard( "FFile_Move_MissingSource", true );
    const auto Source = Guard.Path / "missing.txt";
    const auto Dest   = Guard.Path / "dest.txt";
    const auto Result = Lumen::Filesystem::FFile::Move( Source, Dest );
    LUMEN_CHECK_HAS_ERROR( Result );
}

/**
 * FFile::GetSize
 */

LUMEN_TEST( FFile_GetSize, ReturnsCorrectSize )
{
    const FScopedTestDir Guard( "FFile_GetSize_Success", true );
    const auto FilePath = Guard.Path / "size.txt";
    Lumen::Filesystem::FFile::WriteAllText( FilePath, "12345" );
    const auto Result = Lumen::Filesystem::FFile::GetSize( FilePath );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, 5ULL );
}

LUMEN_TEST( FFile_GetSize, FailsForMissingFile )
{
    const FScopedTestDir Guard( "FFile_GetSize_Missing", true );
    const auto FilePath = Guard.Path / "missing.txt";
    const auto Result   = Lumen::Filesystem::FFile::GetSize( FilePath );
    LUMEN_CHECK_HAS_ERROR( Result );
}

} // namespace
