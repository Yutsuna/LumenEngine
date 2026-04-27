/**
 * @file File.cpp
 * @brief Implementations for file handling utilities in Lumen Engine.
 */

#include "Container/File.hpp"
#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "CoreTypes.hpp"
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

const LumenEngine::FLogCategory LumenEngine::FIOFile::LogIOFile( "IOFile" );

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

LumenEngine::TOptional<LumenEngine::FString> LumenEngine::FIOFile::ReadAllText ( const FString &FilePath ) noexcept
{
    TOptional<TVector<AnsiChar>> FileContentOpt = ReadAllBytes<AnsiChar>( FilePath );

    if ( not FileContentOpt )
    {
        return std::nullopt;
    }

    const TVector<AnsiChar> &FileContent = *FileContentOpt;
    return FString( FileContent.data(), FileContent.size() );
}

LumenEngine::Bool LumenEngine::FIOFile::WriteAllText ( const FString &FilePath, const FString &Text ) noexcept
{
    std::ofstream File( FilePath.c_str(), std::ios::trunc );

    if ( not File.is_open() )
    {
        return false;
    }

    File << Text;
    return File.good();
}