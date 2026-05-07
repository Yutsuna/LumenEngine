/**
 * @file MemoryMappedFile.cpp
 * @brief Implementation of memory-mapped file operations for Linux.
 */

#include "Filesystem/MemoryMappedFile.hpp"
#include "Container/UniquePtr.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Dtor
 */

LumenEngine::Filesystem::FMemoryMappedFile::~FMemoryMappedFile () noexcept
{
    Close();
}

/**
 * Movable
 */

LumenEngine::Filesystem::FMemoryMappedFile::FMemoryMappedFile ( FMemoryMappedFile &&InOther ) noexcept
    : MappedData( InOther.MappedData ), MappedSize( InOther.MappedSize ), FileDescriptor( InOther.FileDescriptor )
{
    InOther.MappedData     = nullptr;
    InOther.MappedSize     = 0;
    InOther.FileDescriptor = -1;
}

LumenEngine::Filesystem::FMemoryMappedFile &LumenEngine::Filesystem::FMemoryMappedFile::operator=( FMemoryMappedFile &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        Close();
        MappedData     = InOther.MappedData;
        MappedSize     = InOther.MappedSize;
        FileDescriptor = InOther.FileDescriptor;

        InOther.MappedData     = nullptr;
        InOther.MappedSize     = 0;
        InOther.FileDescriptor = -1;
    }
    return *this;
}

/**
 * Static
 */

LumenEngine::TExpected<LumenEngine::TUniquePtr<LumenEngine::Filesystem::FMemoryMappedFile>, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FMemoryMappedFile::Open ( const FPath &InPath ) noexcept
{
    const Int32 FD = open( InPath.ToString().c_str(), O_RDONLY | O_CLOEXEC, 0 );

    if ( FD == -1 )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    struct stat St{};
    if ( fstat( FD, &St ) == -1 )
    {
        close( FD );
        return MakeUnexpected( EErrorCode::Failure );
    }

    if ( not S_ISREG( St.st_mode ) )
    {
        close( FD );
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    const USize Size = static_cast<USize>( St.st_size );
    if ( Size == 0 )
    {
        close( FD );
        return MakeUnexpected( EErrorCode::InvalidArgument );
    }

    void *Data = mmap( nullptr, Size, PROT_READ, MAP_PRIVATE, FD, 0 );
    if ( Data == MAP_FAILED )
    {
        close( FD );
        return MakeUnexpected( EErrorCode::Failure );
    }

    TUniquePtr<FMemoryMappedFile> Instance = MakeUnique<FMemoryMappedFile>();
    if ( not Instance )
    {
        munmap( Data, Size );
        close( FD );
        return MakeUnexpected( EErrorCode::OutOfMemory );
    }

    Instance->MappedData     = Data;
    Instance->MappedSize     = Size;
    Instance->FileDescriptor = FD;

    return Instance;
}

/**
 * Public
 */

LumenEngine::TSpan<const LumenEngine::Byte> LumenEngine::Filesystem::FMemoryMappedFile::GetRegion () const noexcept
{
    return { static_cast<const Byte *>( MappedData ), MappedSize };
}

LumenEngine::USize LumenEngine::Filesystem::FMemoryMappedFile::GetSize () const noexcept
{
    return MappedSize;
}

LumenEngine::Bool LumenEngine::Filesystem::FMemoryMappedFile::IsMapped () const noexcept
{
    return MappedData != nullptr;
}

void LumenEngine::Filesystem::FMemoryMappedFile::Close () noexcept
{
    if ( MappedData != nullptr )
    {
        munmap( MappedData, MappedSize );
        MappedData = nullptr;
        MappedSize = 0;
    }

    if ( FileDescriptor != -1 )
    {
        close( FileDescriptor );
        FileDescriptor = -1;
    }
}
