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

LumenEngine::Filesystem::FDirectoryIterator::FDirectoryIterator ( const LumenEngine::Filesystem::FPath &InDir, LumenEngine::Bool bSkipPermissionErrors ) noexcept
{
    State           = LumenEngine::MakeShared<LumenEngine::Filesystem::FDirectoryIterator::FInternalState>();
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

LumenEngine::Filesystem::FDirectoryIterator::~FDirectoryIterator () noexcept = default;

LumenEngine::Filesystem::FDirectoryIterator::FDirectoryIterator ( const LumenEngine::Filesystem::FDirectoryIterator &InOther ) noexcept = default;
LumenEngine::Filesystem::FDirectoryIterator &
LumenEngine::Filesystem::FDirectoryIterator::operator=( const LumenEngine::Filesystem::FDirectoryIterator &InOther ) noexcept = default;

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

LumenEngine::Bool LumenEngine::Filesystem::FDirectoryIterator::operator==( const LumenEngine::Filesystem::FDirectoryIterator &InOther ) const noexcept
{
    return bIsEnd == InOther.bIsEnd;
}

LumenEngine::Bool LumenEngine::Filesystem::FDirectoryIterator::operator!=( const LumenEngine::Filesystem::FDirectoryIterator &InOther ) const noexcept
{
    return not( *this == InOther );
}

LumenEngine::Filesystem::FDirectoryIterator LumenEngine::Filesystem::FDirectoryIterator::Begin () const noexcept
{
    return *this;
}

LumenEngine::Filesystem::FDirectoryIterator LumenEngine::Filesystem::FDirectoryIterator::End () const noexcept
{
    LumenEngine::Filesystem::FDirectoryIterator EndIterator;
    EndIterator.bIsEnd = true;
    return EndIterator;
}

void LumenEngine::Filesystem::FDirectoryIterator::Advance () noexcept
{
    if ( bIsEnd or not State )
        return;

    auto InfoResult = LumenEngine::Filesystem::FFile::GetInfo( LumenEngine::Filesystem::FPath( State->Iterator->path().string() ) );
    if ( InfoResult )
    {
        CurrentInfo = InfoResult.value();
    }
}

// --- FDirectory Implementation ---

LumenEngine::Bool LumenEngine::Filesystem::FDirectory::Exists ( const LumenEngine::Filesystem::FPath &InPath ) noexcept
{
    std::error_code ErrorCode;
    return std::filesystem::is_directory( InPath.ToString(), ErrorCode ) and not ErrorCode;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::Create ( const LumenEngine::Filesystem::FPath &InPath ) noexcept
{
    std::error_code ErrorCode;
    if ( not std::filesystem::create_directory( InPath.ToString(), ErrorCode ) and ErrorCode )
        return LumenEngine::MakeUnexpected( LumenEngine::EErrorCode::Failure );
    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type>
LumenEngine::Filesystem::FDirectory::CreateDirectories ( const LumenEngine::Filesystem::FPath &InPath ) noexcept
{
    std::error_code ErrorCode;
    if ( not std::filesystem::create_directories( InPath.ToString(), ErrorCode ) and ErrorCode )
        return LumenEngine::MakeUnexpected( LumenEngine::EErrorCode::Failure );
    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::Delete ( const LumenEngine::Filesystem::FPath &InPath,
                                                                                                          LumenEngine::Bool bInRecursive ) noexcept
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
        return LumenEngine::MakeUnexpected( LumenEngine::EErrorCode::Failure );
    return {};
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo>, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::GetFiles (
    const LumenEngine::Filesystem::FPath &InPath, LumenEngine::Bool bInRecursive, const LumenEngine::Filesystem::FFilterPredicate &InFilter ) noexcept
{
    std::error_code ErrorCode;
    if ( not std::filesystem::is_directory( InPath.ToString(), ErrorCode ) or ErrorCode )
        return LumenEngine::MakeUnexpected( LumenEngine::EErrorCode::NotFound );

    LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo> FileInfos;

    if ( bInRecursive )
    {
        for ( const auto &DirectoryEntry :
              std::filesystem::recursive_directory_iterator( InPath.ToString(), std::filesystem::directory_options::skip_permission_denied, ErrorCode ) )
        {
            if ( ErrorCode )
                continue;
            if ( DirectoryEntry.is_regular_file( ErrorCode ) )
            {
                auto InfoResult = LumenEngine::Filesystem::FFile::GetInfo( LumenEngine::Filesystem::FPath( DirectoryEntry.path().string() ) );
                if ( InfoResult and ( not InFilter or InFilter( InfoResult.value() ) ) )
                {
                    FileInfos.push_back( InfoResult.value() );
                }
            }
        }
    }
    else
    {
        for ( auto DirectoryIt = LumenEngine::Filesystem::FDirectoryIterator( InPath ); DirectoryIt != DirectoryIt.End(); ++DirectoryIt )
        {
            if ( not DirectoryIt->IsDirectory() and ( not InFilter or InFilter( *DirectoryIt ) ) )
            {
                FileInfos.push_back( *DirectoryIt );
            }
        }
    }

    return FileInfos;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo>, LumenEngine::EErrorCode::Type> LumenEngine::Filesystem::FDirectory::GetDirectories (
    const LumenEngine::Filesystem::FPath &InPath, LumenEngine::Bool bInRecursive, const LumenEngine::Filesystem::FFilterPredicate &InFilter ) noexcept
{
    std::error_code ErrorCode;
    if ( not std::filesystem::is_directory( InPath.ToString(), ErrorCode ) or ErrorCode )
        return LumenEngine::MakeUnexpected( LumenEngine::EErrorCode::NotFound );

    LumenEngine::TVector<LumenEngine::Filesystem::FFileInfo> DirectoryInfos;

    if ( bInRecursive )
    {
        for ( const auto &DirectoryEntry :
              std::filesystem::recursive_directory_iterator( InPath.ToString(), std::filesystem::directory_options::skip_permission_denied, ErrorCode ) )
        {
            if ( ErrorCode )
                continue;
            if ( DirectoryEntry.is_directory( ErrorCode ) )
            {
                auto InfoResult = LumenEngine::Filesystem::FFile::GetInfo( LumenEngine::Filesystem::FPath( DirectoryEntry.path().string() ) );
                if ( InfoResult and ( not InFilter or InFilter( InfoResult.value() ) ) )
                {
                    DirectoryInfos.push_back( InfoResult.value() );
                }
            }
        }
    }
    else
    {
        for ( auto DirectoryIt = LumenEngine::Filesystem::FDirectoryIterator( InPath ); DirectoryIt != DirectoryIt.End(); ++DirectoryIt )
        {
            if ( DirectoryIt->IsDirectory() and ( not InFilter or InFilter( *DirectoryIt ) ) )
            {
                DirectoryInfos.push_back( *DirectoryIt );
            }
        }
    }

    return DirectoryInfos;
}
