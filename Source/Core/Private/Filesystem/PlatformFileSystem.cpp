/// @file PlatformFileSystem.cpp
/// @brief High-performance, cross-platform POSIX and Win32 IFileSystem backend.

#include <Core/CoreDefinitions.hpp>

#include <Filesystem/FileTypes.hpp>
#include <Filesystem/IFileHandle.hpp>
#include <Filesystem/IFileSystem.hpp>
#include <Filesystem/Path.hpp>

#include <Memory/UniquePtr.hpp>
#include <Templates/Vector.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cerrno>
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>

    #if defined( LUMEN_PLATFORM_MACOS )
        #include <copyfile.h>
        #include <sys/clonefile.h>
    #endif
#endif

namespace Lumen
{

namespace Filesystem
{

    namespace Internal
    {
        TExpected<void, EFileError> PlatformEnumerateEntries ( const FPath &InPath,
                                                               Bool bRecursive,
                                                               const std::function<Bool( const FPath &, Bool )> &InCallback );
    }

    namespace
    {
#if defined( LUMEN_PLATFORM_WINDOWS )
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
#else
        [[nodiscard]] EFileError MapPlatformError ( const Int32 ErrorCode ) noexcept
        {
            switch ( ErrorCode )
            {
            case ENOENT:
                return EFileError::NotFound;
            case EACCES:
            case EPERM:
                return EFileError::AccessDenied;
            case EEXIST:
                return EFileError::AlreadyExists;
            case ENOSPC:
            case EDQUOT:
                return EFileError::DiskFull;
            case EBADF:
                return EFileError::InvalidHandle;
            case ENAMETOOLONG:
                return EFileError::PathTooLong;
            case EISDIR:
                return EFileError::IsADirectory;
            case ENOTDIR:
                return EFileError::NotADirectory;
            case EIO:
                return EFileError::IoError;
            default:
                return EFileError::Unknown;
            }
        }
#endif
    } // namespace

#if defined( LUMEN_PLATFORM_WINDOWS )

    class FPlatformFileHandle final : public IFileHandle
    {
    public:

        explicit FPlatformFileHandle ( const HANDLE InHandle ) noexcept : FileHandle( InHandle )
        {
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

            LARGE_INTEGER Zero{ 0 }, Result;
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

#else // POSIX (Linux, macOS)

    class FPlatformFileHandle final : public IFileHandle
    {
    public:

        explicit FPlatformFileHandle ( const Int32 InFd ) noexcept : FileDescriptor( InFd )
        {
        }

        ~FPlatformFileHandle () noexcept override
        {
            Unmap();
            if ( FileDescriptor != -1 )
            {
                ::close( FileDescriptor );
            }
        }

        [[nodiscard]] TExpected<USize, EFileError> Read ( TSpan<Byte> OutBuffer ) noexcept override
        {
            if ( FileDescriptor == -1 or OutBuffer.empty() )
            {
                return static_cast<USize>( 0 );
            }

            const ssize_t Result = ::read( FileDescriptor, OutBuffer.data(), OutBuffer.size() );
            if ( Result < 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            return static_cast<USize>( Result );
        }

        [[nodiscard]] TExpected<USize, EFileError> Write ( TSpan<const Byte> InBuffer ) noexcept override
        {
            if ( FileDescriptor == -1 or InBuffer.empty() )
            {
                return static_cast<USize>( 0 );
            }

            const ssize_t Result = ::write( FileDescriptor, InBuffer.data(), InBuffer.size() );
            if ( Result < 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            return static_cast<USize>( Result );
        }

        [[nodiscard]] TExpected<Int64, EFileError> Seek ( const Int64 Offset, const ESeekOrigin Origin ) noexcept override
        {
            if ( FileDescriptor == -1 )
            {
                return MakeUnexpected( EFileError::InvalidHandle );
            }

            Int32 Whence = SEEK_SET;
            if ( Origin == ESeekOrigin::Current )
            {
                Whence = SEEK_CUR;
            }
            else if ( Origin == ESeekOrigin::End )
            {
                Whence = SEEK_END;
            }

            const off_t Result = ::lseek( FileDescriptor, static_cast<off_t>( Offset ), Whence );
            if ( Result == static_cast<off_t>( -1 ) )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            return static_cast<Int64>( Result );
        }

        [[nodiscard]] Int64 Tell () const noexcept override
        {
            if ( FileDescriptor == -1 )
            {
                return -1;
            }

            return static_cast<Int64>( ::lseek( FileDescriptor, 0, SEEK_CUR ) );
        }

        [[nodiscard]] UInt64 Size () const noexcept override
        {
            if ( FileDescriptor == -1 )
            {
                return 0;
            }

            struct stat St;
            if ( ::fstat( FileDescriptor, &St ) != 0 )
            {
                return 0;
            }

            return static_cast<UInt64>( St.st_size );
        }

        TExpected<void, EFileError> Flush () noexcept override
        {
            if ( FileDescriptor == -1 )
            {
                return MakeUnexpected( EFileError::InvalidHandle );
            }

            if ( ::fsync( FileDescriptor ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
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

            void *Mem = ::mmap( nullptr, MappedSize, PROT_READ, MAP_PRIVATE, FileDescriptor, 0 );
            if ( Mem == MAP_FAILED )
            {
                return {};
            }

            MappedData = static_cast<Byte *>( Mem );
            return { MappedData, MappedSize };
        }

        void Unmap () noexcept override
        {
            if ( MappedData != nullptr )
            {
                ::munmap( MappedData, MappedSize );
                MappedData = nullptr;
                MappedSize = 0;
            }
        }

    private:

        Int32 FileDescriptor = -1;
        Byte *MappedData     = nullptr;
        USize MappedSize     = 0;
    };

#endif

    class FPlatformFileSystem final : public IFileSystem
    {
    public:

        ~FPlatformFileSystem () noexcept override = default;

        [[nodiscard]] TExpected<TUniquePtr<IFileHandle>, EFileError>
        OpenFile ( const FPath &InPath, const EFileAccess InAccess, const EFileMode InMode ) noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
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
#else
            Int32 Flags = 0;
            if ( not HasAnyFlags( InAccess, EFileAccess::Write ) )
            {
                Flags |= O_RDONLY;
            }
            else if ( not HasAnyFlags( InAccess, EFileAccess::Read ) )
            {
                Flags |= O_WRONLY;
            }
            else
            {
                Flags |= O_RDWR;
            }

            switch ( InMode )
            {
            case EFileMode::OpenExisting:
                break;
            case EFileMode::CreateAlways:
                Flags |= O_CREAT | O_TRUNC;
                break;
            case EFileMode::OpenOrCreate:
                Flags |= O_CREAT;
                break;
            case EFileMode::Append:
                Flags |= O_CREAT | O_APPEND;
                break;
            }

            const Int32 Fd = ::open( InPath.ToCString(), Flags, 0666 );
            if ( Fd == -1 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            return TUniquePtr<IFileHandle>( new FPlatformFileHandle( Fd ) );
#endif
        }

        [[nodiscard]] Bool FileExists ( const FPath &InPath ) const noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
            const DWORD Attr = ::GetFileAttributesW( Utf8ToWide( InPath.ToString() ).c_str() );
            return ( Attr != INVALID_FILE_ATTRIBUTES ) and not( Attr & FILE_ATTRIBUTE_DIRECTORY );
#else
            struct stat St;
            return ( ::stat( InPath.ToCString(), &St ) == 0 ) and S_ISREG( St.st_mode );
#endif
        }

        [[nodiscard]] Bool DirectoryExists ( const FPath &InPath ) const noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
            const DWORD Attr = ::GetFileAttributesW( Utf8ToWide( InPath.ToString() ).c_str() );
            return ( Attr != INVALID_FILE_ATTRIBUTES ) and ( Attr & FILE_ATTRIBUTE_DIRECTORY );
#else
            struct stat St;
            return ( ::stat( InPath.ToCString(), &St ) == 0 ) and S_ISDIR( St.st_mode );
#endif
        }

        TExpected<void, EFileError> DeleteFile ( const FPath &InPath ) noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
            if ( not ::DeleteFileW( Utf8ToWide( InPath.ToString() ).c_str() ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
#else
            if ( ::unlink( InPath.ToCString() ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
#endif
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
                const auto Result = Internal::PlatformEnumerateEntries( InPath, false,
                                                                        [this] ( const FPath &Entry, const Bool bIsDir ) -> Bool
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

#if defined( LUMEN_PLATFORM_WINDOWS )
            if ( not ::RemoveDirectoryW( Utf8ToWide( InPath.ToString() ).c_str() ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
#else
            if ( ::rmdir( InPath.ToCString() ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
#endif
            return {};
        }

        TExpected<void, EFileError> CreateDirectory ( const FPath &InPath ) noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
            if ( not ::CreateDirectoryW( Utf8ToWide( InPath.ToString() ).c_str(), nullptr ) )
            {
                const DWORD Err = ::GetLastError();
                if ( Err != ERROR_ALREADY_EXISTS )
                {
                    return MakeUnexpected( MapPlatformError( Err ) );
                }
            }
#else
            if ( ::mkdir( InPath.ToCString(), 0755 ) != 0 and errno != EEXIST )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
#endif
            return {};
        }

        TExpected<void, EFileError> CreateDirectories ( const FPath &InPath ) noexcept override
        {
            if ( DirectoryExists( InPath ) )
            {
                return {};
            }

            const FPath Parent = InPath.GetParent();
            if ( not Parent.IsEmpty() and not DirectoryExists( Parent ) )
            {
                const auto Res = CreateDirectories( Parent );
                if ( not Res )
                {
                    return Res;
                }
            }

            return CreateDirectory( InPath );
        }

        TExpected<void, EFileError>
        CopyFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
            if ( not ::CopyFileExW( Utf8ToWide( InSource.ToString() ).c_str(), Utf8ToWide( InDestination.ToString() ).c_str(),
                                    nullptr, nullptr, nullptr, bOverwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS ) )
            {
                return MakeUnexpected( MapPlatformError( ::GetLastError() ) );
            }
            return {};
#elif defined( LUMEN_PLATFORM_MACOS )
            // macOS Fast path: CoW (clonefile APFS)
            if ( ::clonefile( InSource.ToCString(), InDestination.ToCString(), bOverwrite ? 0 : CLONE_NOFOLLOW ) == 0 )
            {
                return {};
            }
            // Fallback standard
            if ( ::copyfile( InSource.ToCString(), InDestination.ToCString(), nullptr,
                             COPYFILE_ALL | ( bOverwrite ? 0 : COPYFILE_EXCL ) ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
            return {};
#else
            // Linux Fast path: copy_file_range (Zero-copy in kernel)
            const Int32 SrcFd = ::open( InSource.ToCString(), O_RDONLY );
            if ( SrcFd == -1 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            const Int32 DstFlags = O_WRONLY | O_CREAT | ( bOverwrite ? O_TRUNC : O_EXCL );
            const Int32 DstFd    = ::open( InDestination.ToCString(), DstFlags, 0666 );
            if ( DstFd == -1 )
            {
                ::close( SrcFd );
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            struct stat St;
            ::fstat( SrcFd, &St );
            USize Remaining = static_cast<USize>( St.st_size );

            while ( Remaining > 0 )
            {
                const ssize_t Copied = ::copy_file_range( SrcFd, nullptr, DstFd, nullptr, Remaining, 0 );
                if ( Copied <= 0 )
                {
                    break;
                }
                Remaining -= static_cast<USize>( Copied );
            }

            ::close( SrcFd );
            ::close( DstFd );
            return {};
#endif
        }

        TExpected<void, EFileError>
        MoveFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
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
#else
            if ( not bOverwrite and FileExists( InDestination ) )
            {
                return MakeUnexpected( EFileError::AlreadyExists );
            }

            if ( ::rename( InSource.ToCString(), InDestination.ToCString() ) == 0 )
            {
                return {};
            }

            if ( errno == EXDEV )
            {
                const auto CopyResult = CopyFile( InSource, InDestination, bOverwrite );
                if ( not CopyResult )
                {
                    return CopyResult;
                }
                return DeleteFile( InSource );
            }

            return MakeUnexpected( MapPlatformError( errno ) );
#endif
        }

        [[nodiscard]] TExpected<FFileStat, EFileError> GetStat ( const FPath &InPath ) const noexcept override
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
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
#else
            struct stat St;
            if ( ::stat( InPath.ToCString(), &St ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            FFileStat Stat;
            Stat.bExists      = true;
            Stat.bIsDirectory = S_ISDIR( St.st_mode );
            Stat.bIsReadOnly  = ( St.st_mode & S_IWUSR ) == 0;
            Stat.FileSize     = static_cast<UInt64>( St.st_size );
    #if defined( LUMEN_PLATFORM_MACOS )
            Stat.LastModifiedTime = static_cast<UInt64>( St.st_mtimespec.tv_sec ) * 1000ULL +
                                    static_cast<UInt64>( St.st_mtimespec.tv_nsec ) / 1000000ULL;
    #else
            Stat.LastModifiedTime =
                static_cast<UInt64>( St.st_mtim.tv_sec ) * 1000ULL + static_cast<UInt64>( St.st_mtim.tv_nsec ) / 1000000ULL;
    #endif
            return Stat;
#endif
        }
    };

    namespace Internal
    {
        TExpected<void, EFileError> PlatformEnumerateEntries ( const FPath &InPath,
                                                               const Bool bRecursive,
                                                               const std::function<Bool( const FPath &, Bool )> &InCallback )
        {
#if defined( LUMEN_PLATFORM_WINDOWS )
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
#else
            DIR *Dir = ::opendir( InPath.ToCString() );
            if ( Dir == nullptr )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            while ( const dirent *Entry = ::readdir( Dir ) )
            {
                const AnsiChar *Name = Entry->d_name;
                if ( Name[0] == '.' and ( Name[1] == '\0' or ( Name[1] == '.' and Name[2] == '\0' ) ) )
                {
                    continue;
                }

                const FPath EntryPath = InPath / FStringView( Name );
                const Bool bIsDir     = ( Entry->d_type == DT_DIR );

                if ( not InCallback( EntryPath, bIsDir ) )
                {
                    break;
                }

                if ( bIsDir and bRecursive )
                {
                    const auto Res = PlatformEnumerateEntries( EntryPath, true, InCallback );
                    if ( not Res )
                    {
                        ::closedir( Dir );
                        return Res;
                    }
                }
            }

            ::closedir( Dir );
            return {};
#endif
        }
    } // namespace Internal

    IFileSystem &GetDefaultFileSystem () noexcept
    {
        static FPlatformFileSystem Instance;
        return Instance;
    }

} // namespace Filesystem

} // namespace Lumen