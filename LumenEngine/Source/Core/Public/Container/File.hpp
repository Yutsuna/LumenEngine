/**
 * @file File.hpp
 * @brief File handling utilities for Lumen Engine.
 */

#pragma once

#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "Logging/LoggingCategory.hpp"

#include <fstream>

namespace LumenEngine
{

namespace FIOFile
{

    extern const FLogCategory LogIOFile;

    /**
     * @brief Opens a file and returns an optional if successful.
     * @param FilePath The path to the file to open.
     * @return An optional containing the file stream if successful, or std::nullopt if it fails.
     */
    [[nodiscard]] TOptional<std::ifstream> Open ( const FString &FilePath ) noexcept;

    /**
     * @brief Reads all bytes from a file and returns them as a vector.
     * @tparam Type The type of bytes to read (e.g., uint8_t).
     * @param FilePath The path to the file to read.
     * @return An optional containing a vector of bytes if successful, or std::nullopt if it fails.
     */
    template <typename Type> [[nodiscard]] TOptional<TVector<Type>> ReadAllBytes ( const FString &FilePath ) noexcept;

    /**
     * @brief Reads all text from a file and returns it as a string.
     * @param FilePath The path to the file to read.
     * @return An optional containing the file content if successful, or std::nullopt if it fails.
     */
    [[nodiscard]] TOptional<FString> ReadAllText ( const FString &FilePath ) noexcept;

} // namespace FIOFile

} // namespace LumenEngine

#include "Inline/File.inl"
