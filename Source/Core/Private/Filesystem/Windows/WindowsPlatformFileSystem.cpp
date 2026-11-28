/**
 * @file WindowsPlatformFileSystem.cpp
 * @brief Windows platform file handle implementation.
 */

#include <Core/CoreDefinitions.hpp>

#include <Filesystem/Directory.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Filesystem/IFileHandle.hpp>
#include <Filesystem/IFileSystem.hpp>
#include <Filesystem/Path.hpp>

#include <Memory/UniquePtr.hpp>
#include <Templates/Function.hpp>
#include <Templates/Vector.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
#else

extern "C"
{
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
}

/** windows.h defines these as A/W aliases, which collide with the IFileSystem virtual methods. */
    #undef DeleteFile
    #undef CreateDirectory
    #undef CopyFile
    #undef MoveFile

/**
 * Private
 */

namespace Lumen
{

namespace Filesystem
{

    namespace
    {

        /** @brief Maps a platform error code to a Lumen file error. */
        [[nodiscard]] EFileError MapPlatformError ( const DWORD ErrorCode ) noexcept
        {
            switch ( ErrorCode )
            {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return EFileError::NotFound;
            case ERROR_ACCESS_DENIED:
                return EFileError::AccessDenied;
            case ERROR_ALREADY_EXISTS:
            case ERROR_FILE_EXISTS:
                return EFileError::AlreadyExists;
            case ERROR_DISK_FULL:
            case ERROR_HANDLE_DISK_FULL:
                return EFileError::DiskFull;
            case ERROR_INVALID_HANDLE:
                return EFileError::InvalidHandle;
            case ERROR_FILENAME_EXCED_RANGE:
                return EFileError::PathTooLong;
            case ERROR_SHARING_VIOLATION:
            case ERROR_LOCK_VIOLATION:
                return EFileError::SharingViolation;
            case ERROR_DIRECTORY:
                return EFileError::NotADirectory;
            default:
                return EFileError::Unknown;
            }
        }

        [[nodiscard]] std::wstring Utf8ToWide ( const FStringView InUtf8 )
        {
            if ( InUtf8.empty() )
            {
                return {};
            }

            const Int32 Required =
                ::MultiByteToWideChar( CP_UTF8, 0, InUtf8.data(), static_cast<Int32>( InUtf8.size() ), nullptr, 0 );
            std::wstring Result( static_cast<USize>( Required ), L'\0' );
            ::MultiByteToWideChar( CP_UTF8, 0, InUtf8.data(), static_cast<Int32>( InUtf8.size() ), Result.data(), Required );
            return Result;
        }

        [[nodiscard]] FString WideToUtf8 ( const wchar_t *InWide )
        {
            if ( InWide == nullptr or *InWide == L'\0' )
            {
                return {};
            }

            const Int32 Required = ::WideCharToMultiByte( CP_UTF8, 0, InWide, -1, nullptr, 0, nullptr, nullptr );
            FString Result;
            Result.resize( static_cast<USize>( Required - 1 ) );
            ::WideCharToMultiByte( CP_UTF8, 0, InWide, -1, Result.data(), Required, nullptr, nullptr );
            return Result;
        }

    } // namespace

    /**
     * PlatformFileHandle
     */

    /**
     * @class FPlatformFileHandle
     * @brief Represents a platform file handle.
     */
    class FPlatformFileHandle final : public IFileHandle
    {
    public:

        explicit FPlatformFileHandle ( const HANDLE InHandle ) noexcept : FileHandle( InHandle )
        {
            /* */
        }

        ~FPlatformFileHandle () noexcept override
        {
            Unmap();
            if ( FileHandle != INVALID_HANDLE_VALUE )
            {
                ::CloseHandle( FileHandle );
            }
        }

        [[nodiscard]] TExpected<USize, EFileError> Read ( TSpan<Byte> OutBuffer ) noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE or OutBuffer.empty() )
            {
                return static_cast<USize>( 0 );
            }

            DWORD BytesRead    = 0;
            const DWORD ToRead = static_cast<DWORD>( std::min<USize>( OutBuffer.size(), std::numeric_limits<DWORD>::max() ) );
            if ( not ::ReadFile( FileHandle, OutBuffer.data(), ToRead, &BytesRead, nullptr ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            return static_cast<USize>( BytesRead );
        }

        [[nodiscard]] TExpected<USize, EFileError> Write ( TSpan<const Byte> InBuffer ) noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE or InBuffer.empty() )
            {
                return static_cast<USize>( 0 );
            }

            DWORD BytesWritten  = 0;
            const DWORD ToWrite = static_cast<DWORD>( std::min<USize>( InBuffer.size(), std::numeric_limits<DWORD>::max() ) );
            if ( not ::WriteFile( FileHandle, InBuffer.data(), ToWrite, &BytesWritten, nullptr ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            return static_cast<USize>( BytesWritten );
        }

        [[nodiscard]] TExpected<Int64, EFileError> Seek ( const Int64 Offset, const ESeekOrigin Origin ) noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE )
            {
                return MakeUnexpected( EFileError::InvalidHandle );
            }

            DWORD Method = FILE_BEGIN;
            if ( Origin == ESeekOrigin::Current )
            {
                Method = FILE_CURRENT;
            }
            else if ( Origin == ESeekOrigin::End )
            {
                Method = FILE_END;
            }

            LARGE_INTEGER Distance, Result;
            Distance.QuadPart = Offset;
            if ( not ::SetFilePointerEx( FileHandle, Distance, &Result, Method ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            return static_cast<Int64>( Result.QuadPart );
        }

        [[nodiscard]] Int64 Tell () const noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE )
            {
                return -1;
            }

            LARGE_INTEGER Zero;
            Zero.QuadPart = 0;
            LARGE_INTEGER Result;
            if ( not ::SetFilePointerEx( FileHandle, Zero, &Result, FILE_CURRENT ) )
            {
                return -1;
            }

            return static_cast<Int64>( Result.QuadPart );
        }

        [[nodiscard]] UInt64 Size () const noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE )
            {
                return 0;
            }

            LARGE_INTEGER FileSize;
            if ( not ::GetFileSizeEx( FileHandle, &FileSize ) )
            {
                return 0;
            }

            return static_cast<UInt64>( FileSize.QuadPart );
        }

        TExpected<void, EFileError> Flush () noexcept override
        {
            if ( FileHandle == INVALID_HANDLE_VALUE )
            {
                return MakeUnexpected( EFileError::InvalidHandle );
            }

            if ( not ::FlushFileBuffers( FileHandle ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            return {};
        }

        [[nodiscard]] TSpan<const Byte> Map () noexcept override
        {
            if ( MappedData != nullptr )
            {
                return { MappedData, MappedSize };
            }

            MappedSize = static_cast<USize>( Size() );
            if ( MappedSize == 0 )
            {
                return {};
            }

            MappingHandle = ::CreateFileMappingW( FileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr );
            if ( MappingHandle == nullptr )
            {
                return {};
            }

            MappedData = static_cast<Byte *>( ::MapViewOfFile( MappingHandle, FILE_MAP_READ, 0, 0, 0 ) );
            if ( MappedData == nullptr )
            {
                ::CloseHandle( MappingHandle );
                MappingHandle = nullptr;
                return {};
            }

            return { MappedData, MappedSize };
        }

        void Unmap () noexcept override
        {
            if ( MappedData != nullptr )
            {
                ::UnmapViewOfFile( MappedData );
                MappedData = nullptr;
                MappedSize = 0;
            }

            if ( MappingHandle != nullptr )
            {
                ::CloseHandle( MappingHandle );
                MappingHandle = nullptr;
            }
        }

    private:

        HANDLE FileHandle    = INVALID_HANDLE_VALUE;
        HANDLE MappingHandle = nullptr;
        Byte *MappedData     = nullptr;
        USize MappedSize     = 0;
    };

    /**
     * Platform File System
     */

    /**
     * @class FPlatformFileSystem
     * @brief Represents the platform file system.
     */
    class FPlatformFileSystem final : public IFileSystem
    {
    public:

        ~FPlatformFileSystem () noexcept override = default;

        [[nodiscard]] TExpected<TUniquePtr<IFileHandle>, EFileError>
        OpenFile ( const FPath &InPath, const EFileAccess InAccess, const EFileMode InMode ) noexcept override
        {
            DWORD DesiredAccess = 0;
            if ( HasAnyFlags( InAccess, EFileAccess::Read ) )
            {
                DesiredAccess |= GENERIC_READ;
            }
            if ( HasAnyFlags( InAccess, EFileAccess::Write ) )
            {
                DesiredAccess |= GENERIC_WRITE;
            }

            DWORD Disposition = OPEN_EXISTING;
            switch ( InMode )
            {
            case EFileMode::OpenExisting:
                Disposition = OPEN_EXISTING;
                break;
            case EFileMode::CreateAlways:
                Disposition = CREATE_ALWAYS;
                break;
            case EFileMode::OpenOrCreate:
                Disposition = OPEN_ALWAYS;
                break;
            case EFileMode::Append:
                Disposition = OPEN_ALWAYS;
                break;
            }

            const std::wstring Wide = Utf8ToWide( InPath.ToString() );
            const HANDLE Handle     = ::CreateFileW( Wide.c_str(), DesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                                     Disposition, FILE_ATTRIBUTE_NORMAL, nullptr );

            if ( Handle == INVALID_HANDLE_VALUE )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            auto FileHandle = MakeUnique<FPlatformFileHandle>( Handle );
            if ( InMode == EFileMode::Append )
            {
                ( void )FileHandle->Seek( 0, ESeekOrigin::End );
            }

            return TUniquePtr<IFileHandle>( FileHandle.Release() );
        }

        [[nodiscard]] Bool FileExists ( const FPath &InPath ) const noexcept override
        {
            const DWORD Attr = ::GetFileAttributesW( Utf8ToWide( InPath.ToString() ).c_str() );
            return ( Attr != INVALID_FILE_ATTRIBUTES ) and not( Attr & FILE_ATTRIBUTE_DIRECTORY );
        }

        [[nodiscard]] Bool DirectoryExists ( const FPath &InPath ) const noexcept override
        {
            const DWORD Attr = ::GetFileAttributesW( Utf8ToWide( InPath.ToString() ).c_str() );
            return ( Attr != INVALID_FILE_ATTRIBUTES ) and ( Attr & FILE_ATTRIBUTE_DIRECTORY );
        }

        TExpected<void, EFileError> DeleteFile ( const FPath &InPath ) noexcept override
        {
            if ( not ::DeleteFileW( Utf8ToWide( InPath.ToString() ).c_str() ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
        }

        TExpected<void, EFileError> DeleteDirectory ( const FPath &InPath, const Bool bRecursive ) noexcept override
        {
            if ( not DirectoryExists( InPath ) )
            {
                return MakeUnexpected( EFileError::NotFound );
            }

            if ( bRecursive )
            {
                const auto Result =
                    Internal::PlatformEnumerateEntries( InPath, false, [this] ( const FPath &Entry, const Bool bIsDir ) -> Bool
                {
                    if ( bIsDir )
                    {
                        ( void )DeleteDirectory( Entry, true );
                    }
                    else
                    {
                        ( void )DeleteFile( Entry );
                    }
                    return true;
                } );

                if ( not Result )
                {
                    return Result;
                }
            }

            if ( not ::RemoveDirectoryW( Utf8ToWide( InPath.ToString() ).c_str() ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
        }

        TExpected<void, EFileError> CreateDirectory ( const FPath &InPath ) noexcept override
        {
            if ( not ::CreateDirectoryW( Utf8ToWide( InPath.ToString() ).c_str(), nullptr ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
        }

        TExpected<void, EFileError> CreateDirectories ( const FPath &InPath ) noexcept override // NOLINT(misc-no-recursion)
        {
            if ( DirectoryExists( InPath ) )
            {
                return {};
            }

            const FPath Parent = InPath.GetParent();
            if ( not Parent.IsEmpty() and not DirectoryExists( Parent ) )
            {
                const TExpected<void, EFileError> Res = CreateDirectories( Parent );
                if ( not Res )
                {
                    return Res;
                }
            }

            TExpected<void, EFileError> Res = CreateDirectory( InPath );
            if ( not Res and Res.Error() == EFileError::AlreadyExists )
            {
                return {};
            }
            return Res;
        }

        TExpected<void, EFileError>
        CopyFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
        {
            if ( not ::CopyFileExW( Utf8ToWide( InSource.ToString() ).c_str(), Utf8ToWide( InDestination.ToString() ).c_str(),
                                    nullptr, nullptr, nullptr, bOverwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
        }

        TExpected<void, EFileError>
        MoveFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
        {
            DWORD Flags = MOVEFILE_COPY_ALLOWED;
            if ( bOverwrite )
            {
                Flags |= MOVEFILE_REPLACE_EXISTING;
            }

            if ( not ::MoveFileExW( Utf8ToWide( InSource.ToString() ).c_str(), Utf8ToWide( InDestination.ToString() ).c_str(),
                                    Flags ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
        }

        [[nodiscard]] TExpected<FFileStat, EFileError> GetStat ( const FPath &InPath ) const noexcept override
        {
            WIN32_FILE_ATTRIBUTE_DATA Data;
            if ( not ::GetFileAttributesExW( Utf8ToWide( InPath.ToString() ).c_str(), GetFileExInfoStandard, &Data ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            FFileStat Stat;
            Stat.bExists      = true;
            Stat.bIsDirectory = ( Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
            Stat.bIsReadOnly  = ( Data.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) != 0;
            Stat.FileSize     = ( static_cast<UInt64>( Data.nFileSizeHigh ) << 32 ) | Data.nFileSizeLow;

            ULARGE_INTEGER Time;
            Time.LowPart          = Data.ftLastWriteTime.dwLowDateTime;
            Time.HighPart         = Data.ftLastWriteTime.dwHighDateTime;
            Stat.LastModifiedTime = ( Time.QuadPart - 116444736000000000ULL ) / 10000ULL;
            return Stat;
        }
    };

    namespace Internal
    {
        CORE_EXPORT TExpected<void, EFileError> PlatformEnumerateEntries ( // NOLINT(misc-no-recursion)
            const FPath &InPath,
            const Bool bRecursive,
            const TFunction<Bool( const FPath &, Bool )> &InCallback )
        {
            FPath SearchPattern = InPath / "*";
            WIN32_FIND_DATAW FindData;
            const HANDLE Handle = ::FindFirstFileW( Utf8ToWide( SearchPattern.ToString() ).c_str(), &FindData );
            if ( Handle == INVALID_HANDLE_VALUE )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }

            do
            {
                if ( wcscmp( FindData.cFileName, L"." ) == 0 or wcscmp( FindData.cFileName, L".." ) == 0 )
                {
                    continue;
                }

                const Bool bIsDir     = ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
                const FPath EntryPath = InPath / FStringView( WideToUtf8( FindData.cFileName ) );

                if ( not InCallback( EntryPath, bIsDir ) )
                {
                    break;
                }

                if ( bIsDir and bRecursive )
                {
                    const auto Res = PlatformEnumerateEntries( EntryPath, true, InCallback );
                    if ( not Res )
                    {
                        ::FindClose( Handle );
                        return Res;
                    }
                }
            } while ( ::FindNextFileW( Handle, &FindData ) );

            ::FindClose( Handle );
            return {};
        }
    } // namespace Internal

    /**
     * Public
     */

    IFileSystem &GetDefaultFileSystem () noexcept
    {
        static FPlatformFileSystem Instance;
        return Instance;
    }

} // namespace Filesystem

} // namespace Lumen

#endif
