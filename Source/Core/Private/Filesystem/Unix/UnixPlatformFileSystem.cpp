/**
 * @file UnixPlatformFileSystem.cpp
 * @brief Unix platform file system implementation.
 */

#include <Core/CoreDefinitions.hpp>

#include <Filesystem/Directory.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Filesystem/IFileHandle.hpp>
#include <Filesystem/IFileSystem.hpp>
#include <Filesystem/Path.hpp>

#include <Memory/UniquePtr.hpp>
#include <Templates/Vector.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
#else

extern "C"
{
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
}

/**
 * Private
 */

namespace Lumen
{

namespace Filesystem
{

    /**
     * Private
     */

    namespace
    {

        /** @brief Maps a platform error code to a Lumen file error. */
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

    } // namespace

    /**
     * Platform File Handle
     */

    /**
     * @class FPlatformFileHandle
     * @brief Represents a platform file handle.
     */
    class FPlatformFileHandle final : public IFileHandle
    {
    public:

        explicit FPlatformFileHandle ( const Int32 InFd ) noexcept : FileDescriptor( InFd )
        {
            /* */
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

            const ISize Result = ::read( FileDescriptor, OutBuffer.data(), OutBuffer.size() );
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

            const ISize Result = ::write( FileDescriptor, InBuffer.data(), InBuffer.size() );
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

            struct stat St{};
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

    /**
     * Platform System
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

            /** NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg) */
            const Int32 Fd = ::open( InPath.ToCString(), Flags, 0666 );
            if ( Fd == -1 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
            return TUniquePtr<IFileHandle>( new FPlatformFileHandle( Fd ) );
        }

        [[nodiscard]] Bool FileExists ( const FPath &InPath ) const noexcept override
        {
            struct stat St{};
            return ( ::stat( InPath.ToCString(), &St ) == 0 ) and S_ISREG( St.st_mode );
        }

        [[nodiscard]] Bool DirectoryExists ( const FPath &InPath ) const noexcept override
        {
            struct stat St{};
            return ( ::stat( InPath.ToCString(), &St ) == 0 ) and S_ISDIR( St.st_mode );
        }

        TExpected<void, EFileError> DeleteFile ( const FPath &InPath ) noexcept override
        {
            if ( ::unlink( InPath.ToCString() ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
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

            if ( ::rmdir( InPath.ToCString() ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
            return {};
        }

        TExpected<void, EFileError> CreateDirectory ( const FPath &InPath ) noexcept override
        {
            if ( ::mkdir( InPath.ToCString(), 0755 ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
            return {};
        }

        TExpected<void, EFileError> CreateDirectories ( const FPath &InPath ) noexcept override /** NOLINT(misc-no-recursion) */
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
    #if defined( LUMEN_PLATFORM_MACOS )
            /** macOS Fast path: clonefile APFS */
            if ( ::clonefile( InSource.ToCString(), InDestination.ToCString(), bOverwrite ? 0 : CLONE_NOFOLLOW ) == 0 )
            {
                return {};
            }
            /** Fallback standard */
            if ( ::copyfile( InSource.ToCString(), InDestination.ToCString(), nullptr,
                             COPYFILE_ALL | ( bOverwrite ? 0 : COPYFILE_EXCL ) ) != 0 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }
            return {};
    #else
            /** Linux Fast path: copy_file_range */
            /** NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg) */
            const Int32 SrcFd = ::open( InSource.ToCString(), O_RDONLY );
            if ( SrcFd == -1 )
            {
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            const Int32 DstFlags = O_WRONLY | O_CREAT | ( bOverwrite ? O_TRUNC : O_EXCL );
            /** NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg) */
            const Int32 DstFd = ::open( InDestination.ToCString(), DstFlags, 0666 );
            if ( DstFd == -1 )
            {
                ::close( SrcFd );
                return MakeUnexpected( MapPlatformError( errno ) );
            }

            struct stat St{};
            ::fstat( SrcFd, &St );
            USize Remaining = static_cast<USize>( St.st_size );

            while ( Remaining > 0 )
            {
                const ISize Copied = ::copy_file_range( SrcFd, nullptr, DstFd, nullptr, Remaining, 0 );
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
                auto CopyResult = CopyFile( InSource, InDestination, bOverwrite );
                if ( not CopyResult )
                {
                    return CopyResult;
                }
                return DeleteFile( InSource );
            }

            return MakeUnexpected( MapPlatformError( errno ) );
        }

        [[nodiscard]] TExpected<FFileStat, EFileError> GetStat ( const FPath &InPath ) const noexcept override
        {
            struct stat St{};
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
            Stat.LastModifiedTime = ( static_cast<UInt64>( St.st_mtim.tv_sec ) * 1000ULL ) +
                                    ( static_cast<UInt64>( St.st_mtim.tv_nsec ) / 1000000ULL );
    #endif
            return Stat;
        }
    };

    namespace Internal
    {
        CORE_EXPORT TExpected<void, EFileError>
        PlatformEnumerateEntries ( /** NOLINT(misc-no-recursion) */
                                   const FPath &InPath,
                                   const Bool bRecursive,
                                   const std::function<Bool( const FPath &, Bool )> &InCallback )
        {
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
        }
    } // namespace Internal

    IFileSystem &GetDefaultFileSystem () noexcept
    {
        static FPlatformFileSystem Instance;
        return Instance;
    }

} // namespace Filesystem

} // namespace Lumen

#endif
