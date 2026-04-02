/**
 * @file File.cpp
 * @brief Implementations for file handling utilities in Lumen Engine.
 */

#include "Container/File.hpp"
#include "Logging/Logger.hpp"

LumenEngine::TOptional<std::ifstream> LumenEngine::FIOFile::Open ( const FString &FilePath ) noexcept
{
    std::ifstream File( FilePath.c_str(), std::ios::ate | std::ios::binary );

    if ( not File.is_open() )
    {
        LUMEN_LOG_ERROR( LogIOFile, "Failed to open file: {}", FilePath.c_str() );
        return std::nullopt;
    }

    return File;
}
