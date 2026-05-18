/**
 * @file FileSystem.cpp
 * @brief Implementation of global filesystem operations.
 */

#include "Filesystem/FileSystem.hpp"

#include <filesystem>

LumenEngine::TExpected<LumenEngine::Filesystem::FDiskSpaceInfo, LumenEngine::EErrorCode::Type>
LumenEngine::FFileSystem::GetDiskSpace ( const Filesystem::FPath &InPath ) noexcept
{
    std::error_code Ec;
    const std::filesystem::space_info Space = std::filesystem::space( InPath.ToString(), Ec );

    if ( Ec )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    Filesystem::FDiskSpaceInfo Info = { /**/
                                        .Capacity = static_cast<USize>( Space.capacity ),
                                        /**/
                                        .Free = static_cast<USize>( Space.free ),
                                        /**/
                                        .Available = static_cast<USize>( Space.available ) };

    return Info;
}

LumenEngine::Filesystem::FPath LumenEngine::FFileSystem::GetTempDirectory () noexcept
{
    std::error_code Ec;

    return { std::filesystem::temp_directory_path( Ec ).string() };
}

LumenEngine::TExpected<LumenEngine::Filesystem::FPath, LumenEngine::EErrorCode::Type> LumenEngine::FFileSystem::GetCurrentDirectory () noexcept
{
    std::error_code Ec;
    const std::filesystem::path CurrentPath = std::filesystem::current_path( Ec );

    if ( Ec )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    return Filesystem::FPath( CurrentPath.string() );
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::FFileSystem::SetCurrentDirectory ( const LumenEngine::Filesystem::FPath &InPath ) noexcept
{
    std::error_code Ec;
    std::filesystem::current_path( InPath.ToString(), Ec );

    if ( Ec )
    {
        return MakeUnexpected( EErrorCode::Failure );
    }

    return {};
}