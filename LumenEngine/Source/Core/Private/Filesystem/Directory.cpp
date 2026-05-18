/**
 * @file Directory.cpp
 * @brief Implementation of directory operations.
 */

#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"

#include <filesystem>

/**
 * Directory Iterator
 */

struct LumenEngine::Filesystem::FDirectoryIterator::FInternalState
{
    std::filesystem::directory_iterator Iterator;
    std::error_code ErrorCode;
};

/**
 * Ctor
 */

LumenEngine::Filesystem::FDirectoryIterator::FDirectoryIterator ( const FPath &InDir, Bool bSkipPermissionErrors ) noexcept
{
    State           = MakeShared<LumenEngine::Filesystem::FDirectoryIterator::FInternalState>();
    State->Iterator = std::filesystem::directory_iterator(
        InDir.ToString(), bSkipPermissionErrors ? std::filesystem::directory_options::skip_permission_denied : std::filesystem::directory_options::none,
        State->ErrorCode );

    if ( State->ErrorCode or State->Iterator == std::filesystem::directory_iterator() )
    {
        bIsEnd = true;
    }
    else
    {
        bIsEnd = false;
        Advance();
    }
}

/**
 * Dtor
 */

LumenEngine::Filesystem::FDirectoryIterator::~FDirectoryIterator () noexcept = default;

/**
 * Copy
 */

LumenEngine::Filesystem::FDirectoryIterator::FDirectoryIterator ( const FDirectoryIterator &InOther ) noexcept                                    = default;
LumenEngine::Filesystem::FDirectoryIterator &LumenEngine::Filesystem::FDirectoryIterator::operator=( const FDirectoryIterator &InOther ) noexcept = default;

/**
 * Iterator Operators
 */

LumenEngine::Filesystem::FDirectoryIterator::Reference LumenEngine::Filesystem::FDirectoryIterator::operator*() const noexcept
{
    return CurrentInfo;
}

LumenEngine::Filesystem::FDirectoryIterator::Pointer LumenEngine::Filesystem::FDirectoryIterator::operator->() const noexcept
{
    return &CurrentInfo;
}

LumenEngine::Filesystem::FDirectoryIterator &LumenEngine::Filesystem::FDirectoryIterator::operator++() noexcept
{
    if ( not bIsEnd and State and State->Iterator != std::filesystem::directory_iterator() )
    {
        State->Iterator.increment( State->ErrorCode );
        if ( State->ErrorCode or State->Iterator == std::filesystem::directory_iterator() )
        {
            bIsEnd = true;
        }
        else
        {
            Advance();
        }
    }
    return *this;
}

LumenEngine::Bool LumenEngine::Filesystem::FDirectoryIterator::operator==( const FDirectoryIterator &InOther ) const noexcept
{
    return bIsEnd == InOther.bIsEnd;
}

LumenEngine::Bool LumenEngine::Filesystem::FDirectoryIterator::operator!=( const FDirectoryIterator &InOther ) const noexcept
{
    return not( *this == InOther );
}

LumenEngine::Filesystem::FDirectoryIterator LumenEngine::Filesystem::FDirectoryIterator::Begin () const noexcept
{
    return *this;
}

LumenEngine::Filesystem::FDirectoryIterator LumenEngine::Filesystem::FDirectoryIterator::End () noexcept
{
    FDirectoryIterator EndIterator;

    EndIterator.bIsEnd = true;
    return EndIterator;
}

void LumenEngine::Filesystem::FDirectoryIterator::Advance () noexcept
{
    if ( bIsEnd or not State )
    {
        return;
    }

    TExpected<FFileInfo, EErrorCode::Type> InfoResult = FFile::GetInfo( FPath( State->Iterator->path().string() ) );
    if ( InfoResult )
    {
        CurrentInfo = InfoResult.value();
    }
}

/**
 * FDirectory
 */

LumenEngine::Bool LumenEngine::Filesystem::FDirectory::Exists ( const FPath &InPath ) noexcept
{
    std::error_code ErrorCode;

    return std::filesystem::is_directory( InPath.ToString(), ErrorCode ) and not ErrorCode;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::Create ( const FPath &InPath ) noexcept
{
    std::error_code ErrorCode;

    if ( not std::filesystem::create_directory( InPath.ToString(), ErrorCode ) and ErrorCode )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::CreateDirectories ( const FPath &InPath ) noexcept
{
    std::error_code ErrorCode;

    if ( not std::filesystem::create_directories( InPath.ToString(), ErrorCode ) and ErrorCode )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::Delete ( const FPath &InPath, Bool bInRecursive ) noexcept
{
    std::error_code ErrorCode;

    if ( bInRecursive )
    {
        std::filesystem::remove_all( InPath.ToString(), ErrorCode );
    }
    else
    {
        std::filesystem::remove( InPath.ToString(), ErrorCode );
    }

    if ( ErrorCode )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }
    return {};
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo>, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FDirectory::GetFiles ( const Filesystem::FPath &InPath, Bool bInRecursive, const FFilterPredicate &InFilter )
{
    std::error_code ErrorCode;

    if ( not std::filesystem::is_directory( InPath.ToString(), ErrorCode ) or ErrorCode )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    TVector<Filesystem::FFileInfo> FileInfos;

    if ( bInRecursive )
    {
        for ( const auto &DirectoryEntry :
              std::filesystem::recursive_directory_iterator( InPath.ToString(), std::filesystem::directory_options::skip_permission_denied, ErrorCode ) )
        {
            if ( ErrorCode )
            {
                continue;
            }
            if ( DirectoryEntry.is_regular_file( ErrorCode ) )
            {
                auto InfoResult = FFile::GetInfo( FPath( DirectoryEntry.path().string() ) );
                if ( InfoResult and ( not InFilter or InFilter( InfoResult.value() ) ) )
                {
                    FileInfos.push_back( InfoResult.value() );
                }
            }
        }
    }
    else
    {
        for ( auto DirectoryIt = FDirectoryIterator( InPath ); DirectoryIt != DirectoryIt.End(); ++DirectoryIt )
        {
            if ( not DirectoryIt->IsDirectory() and ( not InFilter or InFilter( *DirectoryIt ) ) )
            {
                FileInfos.push_back( *DirectoryIt );
            }
        }
    }

    return FileInfos;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo>, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FDirectory::GetDirectories ( const FPath &InPath, Bool bInRecursive, const FFilterPredicate &InFilter )
{
    std::error_code ErrorCode;
    if ( not std::filesystem::is_directory( InPath.ToString(), ErrorCode ) or ErrorCode )
    {
        return MakeUnexpected( EErrorCode::NotFound );
    }

    TVector<FFileInfo> DirectoryInfos;

    if ( bInRecursive )
    {
        for ( const auto &DirectoryEntry :
              std::filesystem::recursive_directory_iterator( InPath.ToString(), std::filesystem::directory_options::skip_permission_denied, ErrorCode ) )
        {
            if ( ErrorCode )
            {
                continue;
            }
            if ( DirectoryEntry.is_directory( ErrorCode ) )
            {
                auto InfoResult = FFile::GetInfo( FPath( DirectoryEntry.path().string() ) );
                if ( InfoResult and ( not InFilter or InFilter( InfoResult.value() ) ) )
                {
                    DirectoryInfos.push_back( InfoResult.value() );
                }
            }
        }
    }
    else
    {
        for ( auto DirectoryIt = FDirectoryIterator( InPath ); DirectoryIt != DirectoryIt.End(); ++DirectoryIt )
        {
            if ( DirectoryIt->IsDirectory() and ( not InFilter or InFilter( *DirectoryIt ) ) )
            {
                DirectoryInfos.push_back( *DirectoryIt );
            }
        }
    }

    return DirectoryInfos;
}
