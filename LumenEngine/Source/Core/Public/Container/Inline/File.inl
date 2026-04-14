/**
 * @file File.inl
 * @brief Inline implementations for file handling utilities in Lumen Engine.
 */

#pragma once

#include "Container/File.hpp"
#include "CoreTypes.hpp"
#include "Logging/Logger.hpp"

template <typename Type> LumenEngine::TOptional<LumenEngine::TVector<Type>> LumenEngine::FIOFile::ReadAllBytes ( const FString &FilePath ) noexcept
{
    TOptional<std::ifstream> FileOpt = Open( FilePath );

    if ( !FileOpt.has_value() )
    {
        return std::nullopt;
    }

    std::ifstream &File        = FileOpt.value();
    const std::streamsize Size = File.tellg();
    if ( Size <= 0 )
    {
        LUMEN_LOG_ERROR( LogIOFile, "File is empty or an error occurred: {}", FilePath.c_str() );
        return std::nullopt;
    }

    const USize USizeValue = static_cast<USize>( Size );
    if ( USizeValue % sizeof( Type ) != 0 )
    {
        LUMEN_LOG_ERROR( LogIOFile, "File size is not a multiple of the specified type size: {}", FilePath.c_str() );
        return std::nullopt;
    }

    const USize Count = USizeValue / sizeof( Type );
    TVector<Type> Buffer( Count );

    File.seekg( 0, std::ios::beg );
    if ( !File.read( reinterpret_cast<char *>( Buffer.data() ), static_cast<std::streamsize>( USizeValue ) ) )
    {
        LUMEN_LOG_ERROR( LogIOFile, "Failed to read file: {}", FilePath.c_str() );
        return std::nullopt;
    }

    return Buffer;
}
