/**
 * @file File.cpp
 * @brief Implementation of high-performance file operations.
 */

#include "Filesystem/File.hpp"
#include "Container/Expected.hpp"
#include "ErrorCodes.hpp"

#include <cstdio>
#include <filesystem>
#include <system_error>

namespace
{

std::filesystem::copy_options ConvertCopyOptions ( LumenEngine::Filesystem::ECopyOptions InOptions ) noexcept
{
    std::filesystem::copy_options FsOptions = std::filesystem::copy_options::none;

    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::SkipExisting ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::skip_existing;
    }
    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::OverwriteExisting ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::overwrite_existing;
    }
    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::UpdateExisting ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::update_existing;
    }
    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::Recursive ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::recursive;
    }
    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::CopySymlinks ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::copy_symlinks;
    }
    if ( ( InOptions & LumenEngine::Filesystem::ECopyOptions::SkipSymlinks ) != LumenEngine::Filesystem::ECopyOptions::None )
    {
        FsOptions |= std::filesystem::copy_options::skip_symlinks;
    }

    return FsOptions;
}

constexpr LumenEngine::USize MaxBufferSize = 1024UL * 1024UL * 64UL;

} // namespace

/**
 * Dtor
 */

LumenEngine::Filesystem::FFile::~FFile () noexcept
{
    Close();
}

/**
 * Movable
 */

LumenEngine::Filesystem::FFile::FFile ( FFile &&InOther ) noexcept : FileHandle( InOther.FileHandle )
{
    InOther.FileHandle = nullptr;
}

LumenEngine::Filesystem::FFile &LumenEngine::Filesystem::FFile::operator=( FFile &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        Close();
        FileHandle         = InOther.FileHandle;
        InOther.FileHandle = nullptr;
    }
    return *this;
}

/**
 * Public
 */

LumenEngine::Bool LumenEngine::Filesystem::FFile::Exists ( const FPath &InPath ) noexcept
{
    std::error_code Ec;

    return std::filesystem::exists( InPath.ToString(), Ec ) and not Ec;
}

LumenEngine::TExpected<LumenEngine::TUniquePtr<LumenEngine::Filesystem::FFile>, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FFile::Open ( const FPath &InPath, EFileMode InMode ) noexcept
{
    FString ModeStr;

    if ( ( InMode & EFileMode::ReadWrite ) == EFileMode::ReadWrite )
    {
        ModeStr = ( ( InMode & EFileMode::Truncate ) != EFileMode::None ) ? "wb+" : "rb+";
    }
    else if ( ( InMode & EFileMode::Read ) != EFileMode::None )
    {
        ModeStr = "rb";
    }
    else if ( ( InMode & EFileMode::Write ) != EFileMode::None )
    {
        ModeStr = ( ( InMode & EFileMode::Append ) != EFileMode::None ) ? "ab" : "wb";
    }
    else
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }
    // NOLINTNEXTLINE (misc-const-correctness)
    FILE *RawHandle = nullptr;

#if defined( LUMEN_ENGINE_PLATFORM_WINDOWS )
    if ( fopen_s( &RawHandle, InPath.ToString().c_str(), ModeStr.c_str() ) != 0 )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }
#else
    RawHandle = std::fopen( InPath.ToString().c_str(), ModeStr.c_str() );
    if ( RawHandle == nullptr )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }
#endif

    TUniquePtr<FFile> FileInstance = MakeUnique<FFile>();
    FileInstance->FileHandle       = RawHandle;
    return FileInstance;
}

LumenEngine::TExpected<LumenEngine::USize, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Read ( void *OutBuffer, USize InBytesToRead ) noexcept
{
    if ( FileHandle == nullptr or OutBuffer == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    const USize BytesRead = std::fread( OutBuffer, 1, InBytesToRead, static_cast<FILE *>( FileHandle ) );
    if ( BytesRead < InBytesToRead and ( std::ferror( static_cast<FILE *>( FileHandle ) ) != 0 ) )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    return BytesRead;
}

LumenEngine::TExpected<LumenEngine::USize, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Write ( const void *InBuffer, USize InBytesToWrite ) noexcept
{
    if ( FileHandle == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    if ( InBytesToWrite == 0 )
    {
        return 0;
    }

    if ( InBuffer == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    const USize BytesWritten = std::fwrite( InBuffer, 1, InBytesToWrite, static_cast<FILE *>( FileHandle ) );
    if ( BytesWritten < InBytesToWrite and ( std::ferror( static_cast<FILE *>( FileHandle ) ) != 0 ) )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    return BytesWritten;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Seek ( Int64 InOffset, ESeekOrigin InOrigin ) noexcept
{
    if ( FileHandle == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    Int32 Origin = SEEK_SET;

    if ( InOrigin == ESeekOrigin::Current )
    {
        Origin = SEEK_CUR;
    }
    else if ( InOrigin == ESeekOrigin::End )
    {
        Origin = SEEK_END;
    }

#if defined( LUMEN_ENGINE_PLATFORM_WINDOWS )
    const Int32 Result = _fseeki64( static_cast<FILE *>( FileHandle ), InOffset, Origin );
#else
    const Int32 Result = std::fseek( static_cast<FILE *>( FileHandle ), static_cast<long>( InOffset ), Origin );
#endif

    if ( Result != 0 )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

LumenEngine::TExpected<LumenEngine::USize, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Tell () const noexcept
{
    if ( FileHandle == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

#if defined( LUMEN_ENGINE_PLATFORM_WINDOWS )
    const Int64 Position = _ftelli64( static_cast<FILE *>( FileHandle ) );
#else
    const long Position = std::ftell( static_cast<FILE *>( FileHandle ) );
#endif

    if ( Position < 0 )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return static_cast<USize>( Position );
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Flush () noexcept
{
    if ( FileHandle == nullptr )
    {
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }
    if ( std::fflush( static_cast<FILE *>( FileHandle ) ) != 0 )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

void LumenEngine::Filesystem::FFile::Close () noexcept
{
    if ( FileHandle != nullptr )
    {
        std::fclose( static_cast<FILE *>( FileHandle ) );
        FileHandle = nullptr;
    }
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FFile::Copy ( const FPath &InSource, const FPath &InDestination, ECopyOptions InOptions, const FProgressCallback &InProgressCallback )
{
    std::error_code ErrorCode;

    if ( InProgressCallback == nullptr )
    {
        std::filesystem::copy( InSource.ToString(), InDestination.ToString(), ConvertCopyOptions( InOptions ), ErrorCode );
        if ( ErrorCode )
        {
            return MakeUnexpected( EErrorCode::Failure );
        }
        return {};
    }

    const USize FileSize = std::filesystem::file_size( InSource.ToString(), ErrorCode );
    if ( ErrorCode )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    TExpected<TUniquePtr<Filesystem::FFile>, EErrorCode::Type> SourceFile = Open( InSource, EFileMode::Read );
    LUMEN_EXPECT_VALUE( SourceFile );

    TExpected<TUniquePtr<Filesystem::FFile>, EErrorCode::Type> DestFile = Open( InDestination, EFileMode::Write );
    LUMEN_EXPECT_VALUE( DestFile );

    const TUniquePtr<Byte[]> Buffer = MakeUnique<Byte[]>( MaxBufferSize );

    USize TotalRead = 0;
    while ( TotalRead < FileSize )
    {
        const USize BytesToRead                       = std::min( MaxBufferSize, FileSize - TotalRead );
        TExpected<USize, EErrorCode::Type> ReadResult = SourceFile.value()->Read( Buffer.Get(), BytesToRead );
        LUMEN_EXPECT_VALUE( ReadResult );

        TExpected<USize, EErrorCode::Type> WriteResult = DestFile.value()->Write( Buffer.Get(), ReadResult.value() );
        LUMEN_EXPECT_VALUE( WriteResult );

        TotalRead += ReadResult.value();
        InProgressCallback( TotalRead, FileSize );
    }

    return {};
}

LumenEngine::TExpected<LumenEngine::FString, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::ReadAllText ( const FPath &InPath )
{
    TExpected<TUniquePtr<Filesystem::FFile>, EErrorCode::Type> FileResult = Open( InPath, EFileMode::Read );
    LUMEN_EXPECT_VALUE( FileResult );

    TExpected<FFileInfo, EErrorCode::Type> InfoResult = GetInfo( InPath );
    LUMEN_EXPECT_VALUE( InfoResult );

    const USize Size = InfoResult.value().SizeBytes;
    if ( Size == 0 )
    {
        return FString();
    }

    FString Content;
    Content.resize( Size );

    TExpected<USize, EErrorCode::Type> ReadResult = FileResult.value()->Read( Content.data(), Size );
    LUMEN_EXPECT_VALUE( ReadResult );

    return Content;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::WriteAllText ( const FPath &InPath, const FString &InText ) noexcept
{
    TExpected<TUniquePtr<Filesystem::FFile>, EErrorCode::Type> FileResult = Open( InPath, EFileMode::Write );
    LUMEN_EXPECT_VALUE( FileResult );

    TExpected<USize, EErrorCode::Type> WriteResult = FileResult.value()->Write( InText.data(), InText.size() );
    LUMEN_EXPECT_VALUE( WriteResult );

    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Move ( const FPath &InSource, const FPath &InDestination ) noexcept
{
    std::error_code ErrorCode;
    std::filesystem::rename( InSource.ToString(), InDestination.ToString(), ErrorCode );

    if ( ErrorCode )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::Delete ( const FPath &InPath ) noexcept
{
    std::error_code ErrorCode;
    const Bool bRemoved = std::filesystem::remove( InPath.ToString(), ErrorCode );

    LUMEN_EXPECT( bRemoved, EErrorCode::NotFound );
    return {};
}

LumenEngine::TExpected<LumenEngine::Filesystem::FFileInfo, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::GetInfo ( const FPath &InPath ) noexcept
{
    std::error_code ErrorCode;
    const std::filesystem::path StdPath( InPath.ToString() );
    const Bool bExists = std::filesystem::exists( StdPath, ErrorCode );

    if ( not bExists )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    FFileInfo Info;
    Info.Path      = StdPath.string();
    Info.Name      = StdPath.filename().string();
    Info.Stem      = StdPath.stem().string();
    Info.Extension = StdPath.extension().string();

    if ( std::filesystem::is_regular_file( StdPath, ErrorCode ) )
    {
        Info.SizeBytes = std::filesystem::file_size( StdPath, ErrorCode );
    }

    const auto FileTime = std::filesystem::last_write_time( StdPath, ErrorCode );
    if ( not ErrorCode )
    {
        Info.LastModified = static_cast<Float64>( FileTime.time_since_epoch().count() );
    }

    Info.Attributes = EFileAttributes::None;
    if ( std::filesystem::is_directory( StdPath, ErrorCode ) )
    {
        Info.Attributes |= EFileAttributes::Directory;
    }
    if ( std::filesystem::is_symlink( StdPath, ErrorCode ) )
    {
        Info.Attributes |= EFileAttributes::SymLink;
    }

    const std::filesystem::perms Perms = std::filesystem::status( StdPath, ErrorCode ).permissions();
    if ( ( Perms & std::filesystem::perms::owner_write ) == std::filesystem::perms::none )
    {
        Info.Attributes |= EFileAttributes::ReadOnly;
    }

    return Info;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FFile::SetAttributes ( const FPath &InPath, EFileAttributes InAttributes ) noexcept
{
    std::error_code ErrorCode;
    const std::filesystem::path StdPath( InPath.ToString() );
    const Bool bExists = std::filesystem::exists( StdPath, ErrorCode );

    LUMEN_EXPECT( bExists, EErrorCode::NotFound );

    std::filesystem::perms NewPerms = std::filesystem::status( StdPath, ErrorCode ).permissions();

    if ( ( InAttributes & EFileAttributes::ReadOnly ) != EFileAttributes::None )
    {
        NewPerms &= ~( std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write );
    }
    else
    {
        NewPerms |= ( std::filesystem::perms::owner_write | std::filesystem::perms::group_write );
    }

    std::filesystem::permissions( StdPath, NewPerms, std::filesystem::perm_options::replace, ErrorCode );
    LUMEN_EXPECT( ErrorCode, EErrorCode::Failure );

    return {};
}
