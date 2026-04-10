/**
 * @file File.cpp
 * @brief Implementations for file handling utilities in Lumen Engine.
 */

#include "Container/File.hpp"
#include "Logging/Logger.hpp"

#include <filesystem>

namespace
{
std::filesystem::path GetExecutableDirectory () noexcept
{
#if defined( __linux__ )
    std::error_code ErrorCode;
    const std::filesystem::path ExecutablePath = std::filesystem::read_symlink( "/proc/self/exe", ErrorCode );

    if ( not ErrorCode and not ExecutablePath.empty() )
    {
        return ExecutablePath.parent_path();
    }
#endif

    std::error_code CurrentPathError;
    return std::filesystem::current_path( CurrentPathError );
}
} // namespace

LumenEngine::TOptional<std::ifstream> LumenEngine::FIOFile::Open ( const FString &FilePath ) noexcept
{
    constexpr std::ios::openmode OpenMode = std::ios::ate | std::ios::binary;
    std::ifstream File;

    File.open( FilePath.c_str(), OpenMode );

    if ( not File.is_open() )
    {
        const std::filesystem::path RelativePath( FilePath.c_str() );

        if ( RelativePath.is_relative() )
        {
            const std::filesystem::path FallbackPath = GetExecutableDirectory() / RelativePath;
            File.open( FallbackPath, OpenMode );
        }
    }

    if ( not File.is_open() )
    {
        LUMEN_LOG_ERROR( LogIOFile, "Failed to open file: {}", FilePath.c_str() );
        return std::nullopt;
    }

    return File;
}
