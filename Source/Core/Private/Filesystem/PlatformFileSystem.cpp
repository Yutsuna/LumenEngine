///
/// @file Core/Private/Filesystem/PlatformFileSystem.cpp
/// @brief POSIX-backed IFileSystem and IFileHandle implementation.
///

#include <Filesystem/FileTypes.hpp>
#include <Filesystem/IFileHandle.hpp>
#include <Filesystem/IFileSystem.hpp>
#include <Filesystem/Path.hpp>

#include <Array/Vector.hpp>
#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>

#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace Lumen
{

namespace Filesystem
{

    namespace
    {

        EFileError MapErrnoToFileError ( const int ErrorCode ) noexcept
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
            default:
                return EFileError::Unknown;
            }
        }

        int ToPosixSeekOrigin ( const ESeekOrigin Origin ) noexcept
        {
            switch ( Origin )
            {
            case ESeekOrigin::Begin:
                return SEEK_SET;
            case ESeekOrigin::Current:
                return SEEK_CUR;
            case ESeekOrigin::End:
                return SEEK_END;
            }

            return SEEK_SET;
        }

        int ToPosixOpenFlags ( const EFileAccess InAccess, const EFileMode InMode ) noexcept
        {
            int Flags = 0;

            if ( !HasAnyFlags( InAccess, EFileAccess::Write ) )
            {
                Flags |= O_RDONLY;
            }
            else if ( !HasAnyFlags( InAccess, EFileAccess::Read ) )
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

            return Flags;
        }

        Bool CopyFileContents ( IFileHandle &Source, IFileHandle &Destination ) noexcept
        {
            constexpr USize BufferSize = 64 * 1024; // 64 KiB
            TVector<Byte> Buffer;
            Buffer.resize( BufferSize );

            for ( ;; )
            {
                const USize ReadCount = Source.Read( TSpan<Byte>( Buffer.data(), Buffer.size() ) );
                if ( ReadCount == 0 )
                {
                    break;
                }

                USize TotalWritten = 0;
                while ( TotalWritten < ReadCount )
                {
                    const USize Written =
                        Destination.Write( TSpan<const Byte>( Buffer.data() + TotalWritten, ReadCount - TotalWritten ) );
                    if ( Written == 0 )
                    {
                        return false;
                    }

                    TotalWritten += Written;
                }
            }

            return Destination.Flush();
        }

        Bool CopyDirectoryRecursive ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite )
        {
            IFileSystem &FileSystem = GetDefaultFileSystem();

            if ( !FileSystem.DirectoryExists( InDestination ) )
            {
                if ( !FileSystem.CreateDirectories( InDestination ) )
                {
                    return false;
                }
            }

            DIR *const DirectoryStream = opendir( InSource.ToCString() );
            if ( DirectoryStream == nullptr )
            {
                return false;
            }

            Bool bSuccess = true;
            while ( const dirent *const Entry = readdir( DirectoryStream ) )
            {
                const AnsiChar *const Name = Entry->d_name;

                if ( Name[0] == '.' && ( Name[1] == '\0' || ( Name[1] == '.' && Name[2] == '\0' ) ) )
                {
                    continue;
                }

                FPath SourcePath = InSource;
                SourcePath /= FStringView( Name );
                FPath DestPath = InDestination;
                DestPath /= FStringView( Name );

                struct stat StatBuffer;
                if ( stat( SourcePath.ToCString(), &StatBuffer ) != 0 )
                {
                    bSuccess = false;
                    continue;
                }

                if ( S_ISDIR( StatBuffer.st_mode ) )
                {
                    if ( !CopyDirectoryRecursive( SourcePath, DestPath, bOverwrite ) )
                    {
                        bSuccess = false;
                    }
                }
                else
                {
                    if ( !FileSystem.CopyFile( SourcePath, DestPath, bOverwrite ) )
                    {
                        bSuccess = false;
                    }
                }
            }

            closedir( DirectoryStream );
            return bSuccess;
        }

        class FMappedFileRegion
        {
        public:

            FMappedFileRegion () noexcept = default;

            FMappedFileRegion ( Byte *InData, USize InSize ) noexcept : Data( InData ), Size( InSize )
            {
            }

            FMappedFileRegion ( const FMappedFileRegion & )           = delete;
            FMappedFileRegion &operator=( const FMappedFileRegion & ) = delete;

            FMappedFileRegion ( FMappedFileRegion &&Other ) noexcept
                : Data( std::exchange( Other.Data, nullptr ) ), Size( std::exchange( Other.Size, 0 ) )
            {
            }

            FMappedFileRegion &operator=( FMappedFileRegion &&Other ) noexcept
            {
                if ( this != &Other )
                {
                    Unmap();
                    Data = std::exchange( Other.Data, nullptr );
                    Size = std::exchange( Other.Size, 0 );
                }

                return *this;
            }

            ~FMappedFileRegion () noexcept
            {
                Unmap();
            }

            [[nodiscard]] TSpan<const Byte> AsSpan () const noexcept
            {
                return { Data, Size };
            }

            [[nodiscard]] Bool IsValid () const noexcept
            {
                return Data != nullptr;
            }

            void Unmap () noexcept
            {
                if ( Data != nullptr )
                {
                    munmap( Data, Size );
                    Data = nullptr;
                    Size = 0;
                }
            }

        private:

            Byte *Data = nullptr;
            USize Size = 0;
        };

        class FPlatformFileHandle final : public IFileHandle
        {
        public:

            explicit FPlatformFileHandle ( const int InFileDescriptor ) noexcept : FileDescriptor( InFileDescriptor )
            {
            }

            ~FPlatformFileHandle () noexcept override
            {
                if ( FileDescriptor != -1 )
                {
                    close( FileDescriptor );
                }
            }

            FPlatformFileHandle ( const FPlatformFileHandle & )           = delete;
            FPlatformFileHandle &operator=( const FPlatformFileHandle & ) = delete;

            FPlatformFileHandle ( FPlatformFileHandle &&Other ) noexcept
                : FileDescriptor( std::exchange( Other.FileDescriptor, -1 ) ), MappedRegion( std::move( Other.MappedRegion ) )
            {
            }

            FPlatformFileHandle &operator=( FPlatformFileHandle && ) = delete;

            USize Read ( TSpan<Byte> OutBuffer ) noexcept override
            {
                if ( FileDescriptor == -1 || OutBuffer.empty() )
                {
                    return 0;
                }

                const ssize_t Result = ::read( FileDescriptor, OutBuffer.data(), OutBuffer.size() );
                if ( Result < 0 )
                {
                    return 0;
                }

                return static_cast<USize>( Result );
            }

            USize Write ( TSpan<const Byte> InBuffer ) noexcept override
            {
                if ( FileDescriptor == -1 || InBuffer.empty() )
                {
                    return 0;
                }

                const ssize_t Result = ::write( FileDescriptor, InBuffer.data(), InBuffer.size() );
                if ( Result < 0 )
                {
                    return 0;
                }

                return static_cast<USize>( Result );
            }

            Bool Seek ( const Int64 Offset, const ESeekOrigin Origin ) noexcept override
            {
                if ( FileDescriptor == -1 )
                {
                    return false;
                }

                return ::lseek( FileDescriptor, static_cast<off_t>( Offset ), ToPosixSeekOrigin( Origin ) ) !=
                       static_cast<off_t>( -1 );
            }

            [[nodiscard]] Int64 Tell () const noexcept override
            {
                if ( FileDescriptor == -1 )
                {
                    return -1;
                }

                const off_t Position = ::lseek( FileDescriptor, 0, SEEK_CUR );
                if ( Position == static_cast<off_t>( -1 ) )
                {
                    return -1;
                }

                return static_cast<Int64>( Position );
            }

            [[nodiscard]] UInt64 Size () const noexcept override
            {
                if ( FileDescriptor == -1 )
                {
                    return 0;
                }

                struct stat StatBuffer;
                if ( fstat( FileDescriptor, &StatBuffer ) != 0 )
                {
                    return 0;
                }

                return static_cast<UInt64>( StatBuffer.st_size );
            }

            Bool Flush () noexcept override
            {
                if ( FileDescriptor == -1 )
                {
                    return false;
                }

                return ::fsync( FileDescriptor ) == 0;
            }

            [[nodiscard]] TSpan<const Byte> Map () noexcept override
            {
                if ( MappedRegion.IsValid() )
                {
                    return MappedRegion.AsSpan();
                }

                const UInt64 FileSize = Size();
                if ( FileSize == 0 || FileSize > static_cast<UInt64>( std::numeric_limits<USize>::max() ) )
                {
                    return {};
                }

                void *const Mapping =
                    ::mmap( nullptr, static_cast<USize>( FileSize ), PROT_READ, MAP_PRIVATE, FileDescriptor, 0 );
                if ( Mapping == MAP_FAILED )
                {
                    return {};
                }

                MappedRegion = FMappedFileRegion( static_cast<Byte *>( Mapping ), static_cast<USize>( FileSize ) );
                return MappedRegion.AsSpan();
            }

            void Unmap () noexcept override
            {
                MappedRegion.Unmap();
            }

        private:

            int FileDescriptor = -1;
            FMappedFileRegion MappedRegion;
        };

        class FPlatformFileSystem final : public IFileSystem
        {
        public:

            ~FPlatformFileSystem () noexcept override = default;

            [[nodiscard]] TExpected<TUniquePtr<IFileHandle>, EFileError>
            OpenFile ( const FPath &InPath, const EFileAccess InAccess, const EFileMode InMode ) noexcept override
            {
                const int FileDescriptor = ::open( InPath.ToCString(), ToPosixOpenFlags( InAccess, InMode ), 0666 );
                if ( FileDescriptor == -1 )
                {
                    return MakeUnexpected( MapErrnoToFileError( errno ) );
                }

                return TUniquePtr<IFileHandle>( new FPlatformFileHandle( FileDescriptor ) );
            }

            [[nodiscard]] Bool FileExists ( const FPath &InPath ) const noexcept override
            {
                struct stat StatBuffer;
                return stat( InPath.ToCString(), &StatBuffer ) == 0 && S_ISREG( StatBuffer.st_mode );
            }

            [[nodiscard]] Bool DirectoryExists ( const FPath &InPath ) const noexcept override
            {
                struct stat StatBuffer;
                return stat( InPath.ToCString(), &StatBuffer ) == 0 && S_ISDIR( StatBuffer.st_mode );
            }

            Bool DeleteFile ( const FPath &InPath ) noexcept override
            {
                return ::unlink( InPath.ToCString() ) == 0;
            }

            Bool DeleteDirectory ( const FPath &InPath, const Bool bRecursive ) noexcept override
            {
                if ( !bRecursive )
                {
                    return ::rmdir( InPath.ToCString() ) == 0;
                }

                DIR *const DirectoryStream = opendir( InPath.ToCString() );
                if ( DirectoryStream == nullptr )
                {
                    return false;
                }

                Bool bSuccess = true;
                while ( const dirent *const Entry = readdir( DirectoryStream ) )
                {
                    const AnsiChar *const Name = Entry->d_name;

                    if ( Name[0] == '.' && ( Name[1] == '\0' || ( Name[1] == '.' && Name[2] == '\0' ) ) )
                    {
                        continue;
                    }

                    FPath EntryPath = InPath;
                    EntryPath /= FStringView( Name );

                    struct stat StatBuffer;
                    if ( stat( EntryPath.ToCString(), &StatBuffer ) != 0 )
                    {
                        bSuccess = false;
                        continue;
                    }

                    if ( S_ISDIR( StatBuffer.st_mode ) )
                    {
                        if ( !DeleteDirectory( EntryPath, true ) )
                        {
                            bSuccess = false;
                        }
                    }
                    else
                    {
                        if ( ::unlink( EntryPath.ToCString() ) != 0 )
                        {
                            bSuccess = false;
                        }
                    }
                }

                closedir( DirectoryStream );

                if ( ::rmdir( InPath.ToCString() ) != 0 )
                {
                    bSuccess = false;
                }

                return bSuccess;
            }

            Bool CreateDirectory ( const FPath &InPath ) noexcept override
            {
                return ::mkdir( InPath.ToCString(), 0755 ) == 0;
            }

            Bool CreateDirectories ( const FPath &InPath ) noexcept override
            {
                if ( DirectoryExists( InPath ) )
                {
                    return true;
                }

                const FPath Parent = InPath.GetParent();
                if ( !Parent.IsEmpty() && !DirectoryExists( Parent ) )
                {
                    if ( !CreateDirectories( Parent ) )
                    {
                        return false;
                    }
                }

                return CreateDirectory( InPath );
            }

            Bool CopyFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
            {
                if ( !bOverwrite && FileExists( InDestination ) )
                {
                    return false;
                }

                auto SourceResult = OpenFile( InSource, EFileAccess::Read, EFileMode::OpenExisting );
                if ( !SourceResult )
                {
                    return false;
                }

                auto DestResult = OpenFile( InDestination, EFileAccess::Write, EFileMode::CreateAlways );
                if ( !DestResult )
                {
                    return false;
                }

                return CopyFileContents( *SourceResult->Get(), *DestResult->Get() );
            }

            Bool MoveFile ( const FPath &InSource, const FPath &InDestination, const Bool bOverwrite ) noexcept override
            {
                if ( !bOverwrite && FileExists( InDestination ) )
                {
                    return false;
                }

                if ( ::rename( InSource.ToCString(), InDestination.ToCString() ) == 0 )
                {
                    return true;
                }

                if ( errno != EXDEV )
                {
                    return false;
                }

                // Cross-device move: copy then delete source.
                if ( DirectoryExists( InSource ) )
                {
                    if ( !CopyDirectoryRecursive( InSource, InDestination, bOverwrite ) )
                    {
                        return false;
                    }

                    return DeleteDirectory( InSource, true );
                }

                if ( !CopyFile( InSource, InDestination, bOverwrite ) )
                {
                    return false;
                }

                return DeleteFile( InSource );
            }

            [[nodiscard]] TExpected<FFileStat, EFileError> GetStat ( const FPath &InPath ) const noexcept override
            {
                struct stat StatBuffer;
                if ( stat( InPath.ToCString(), &StatBuffer ) != 0 )
                {
                    return MakeUnexpected( MapErrnoToFileError( errno ) );
                }

                FFileStat Result;
                Result.FileSize         = static_cast<UInt64>( StatBuffer.st_size );
                Result.bIsDirectory     = S_ISDIR( StatBuffer.st_mode ) != 0;
                Result.bIsReadOnly      = ( StatBuffer.st_mode & S_IWUSR ) == 0;
                Result.bExists          = true;
                Result.LastModifiedTime = static_cast<UInt64>( StatBuffer.st_mtim.tv_sec ) * 1000ULL +
                                          static_cast<UInt64>( StatBuffer.st_mtim.tv_nsec ) / 1000000ULL;

                return Result;
            }
        };

    } // namespace

    IFileSystem &GetDefaultFileSystem () noexcept
    {
        static FPlatformFileSystem Instance;
        return Instance;
    }

} // namespace Filesystem

} // namespace Lumen
