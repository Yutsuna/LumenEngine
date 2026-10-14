/// @file File.cpp
/// @brief Monadic FFile helper implementations.

#include <Filesystem/File.hpp>
#include <Filesystem/IFileSystem.hpp>
#include <cstring>
#include <limits>

namespace Lumen
{

namespace Filesystem
{

    Bool FFile::Exists ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().FileExists( InPath );
    }

    TExpected<TVector<Byte>, EFileError> FFile::ReadAllBytes ( const FPath &InPath )
    {
        return GetDefaultFileSystem()
            .OpenFile( InPath, EFileAccess::Read, EFileMode::OpenExisting )
            .AndThen(
                [] ( TUniquePtr<IFileHandle> Handle ) -> TExpected<TVector<Byte>, EFileError>
                {
                    // Fast path : Zero-copy Memory Map si disponible
                    const TSpan<const Byte> MappedSpan = Handle->Map();
                    if ( not MappedSpan.empty() )
                    {
                        TVector<Byte> Buffer;
                        Buffer.resize( MappedSpan.size() );
                        std::memcpy( Buffer.data(), MappedSpan.data(), MappedSpan.size() );
                        Handle->Unmap();
                        return Buffer;
                    }

                    const UInt64 FileSize = Handle->Size();
                    if ( FileSize > static_cast<UInt64>( std::numeric_limits<USize>::max() ) ) [[unlikely]]
                    {
                        return MakeUnexpected( EFileError::PathTooLong );
                    }

                    TVector<Byte> Buffer;
                    Buffer.resize( static_cast<USize>( FileSize ) );

                    USize TotalRead = 0;
                    while ( TotalRead < Buffer.size() )
                    {
                        auto ReadResult = Handle->Read( TSpan<Byte>( Buffer.data() + TotalRead, Buffer.size() - TotalRead ) );
                        if ( not ReadResult )
                        {
                            return MakeUnexpected( ReadResult.Error() );
                        }

                        const USize BytesRead = *ReadResult;
                        if ( BytesRead == 0 )
                        {
                            break;
                        }

                        TotalRead += BytesRead;
                    }

                    Buffer.resize( TotalRead );
                    return Buffer;
                } );
    }

    TExpected<FString, EFileError> FFile::ReadAllText ( const FPath &InPath )
    {
        return ReadAllBytes( InPath ).Transform(
            [] ( const TVector<Byte> &Bytes ) -> FString
            { return FString( reinterpret_cast<const AnsiChar *>( Bytes.data() ), Bytes.size() ); } );
    }

    TExpected<void, EFileError> FFile::WriteAllBytes ( const FPath &InPath, const TSpan<const Byte> InData ) noexcept
    {
        return GetDefaultFileSystem()
            .OpenFile( InPath, EFileAccess::Write, EFileMode::CreateAlways )
            .AndThen(
                [InData] ( TUniquePtr<IFileHandle> Handle ) -> TExpected<void, EFileError>
                {
                    USize TotalWritten = 0;

                    while ( TotalWritten < InData.size() )
                    {
                        auto WriteResult = Handle->Write( InData.subspan( TotalWritten ) );
                        if ( not WriteResult )
                        {
                            return MakeUnexpected( WriteResult.Error() );
                        }

                        const USize Written = *WriteResult;
                        if ( Written == 0 ) [[unlikely]]
                        {
                            return MakeUnexpected( EFileError::DiskFull );
                        }

                        TotalWritten += Written;
                    }

                    return Handle->Flush();
                } );
    }

    TExpected<void, EFileError> FFile::WriteAllText ( const FPath &InPath, const FStringView InContent ) noexcept
    {
        return WriteAllBytes( InPath, TSpan<const Byte>( reinterpret_cast<const Byte *>( InContent.data() ), InContent.size() ) );
    }

    TExpected<void, EFileError> FFile::Delete ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().DeleteFile( InPath );
    }

    TExpected<void, EFileError> FFile::Copy ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept
    {
        return GetDefaultFileSystem().CopyFile( InSource, InDestination, bOverwrite );
    }

    TExpected<void, EFileError> FFile::Move ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept
    {
        return GetDefaultFileSystem().MoveFile( InSource, InDestination, bOverwrite );
    }

    TExpected<UInt64, EFileError> FFile::GetSize ( const FPath &InPath ) noexcept
    {
        return GetDefaultFileSystem().GetStat( InPath ).Transform( [] ( const FFileStat &Stat ) noexcept -> UInt64
                                                                   { return Stat.FileSize; } );
    }

} // namespace Filesystem

} // namespace Lumen