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

    /**
     * @brief Writes all bytes to a file.
     * @tparam Type The type of bytes to write.
     * @param FilePath The path to the file.
     * @param Data The data to write.
     * @return True if successful.
     */
    template <typename Type> [[nodiscard]] Bool WriteAllBytes ( const FString &FilePath, const TVector<Type> &Data ) noexcept;

    /**
     * @brief Writes text to a file.
     * @param FilePath The path to the file.
     * @param Text The text to write.
     * @return True if successful.
     */
    [[nodiscard]] Bool WriteAllText ( const FString &FilePath, const FString &Text ) noexcept;

} // namespace FIOFile

} // namespace LumenEngine

#include "Inline/File.inl"
