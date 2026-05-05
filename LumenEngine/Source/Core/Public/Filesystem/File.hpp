/**
 * @file File.hpp
 * @brief High-level, high-performance file operations and handle encapsulation.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "ErrorCodes.hpp"

#include "Container/Expected.hpp"
#include "Container/UniquePtr.hpp"

#include "Filesystem/Path.hpp"
#include "Filesystem/Types.hpp"
#include "NonCopyable.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    /**
     * @class FFile
     * @brief Encapsulates a file handle for high-performance I/O operations.
     */
    class LUMEN_ENGINE_API FFile final : public FNonCopyable
    {
    public:

        FFile () noexcept = default;
        ~FFile () noexcept;

        FFile ( FFile &&InOther ) noexcept;
        FFile &operator=( FFile &&InOther ) noexcept;

    public:

        /** @return true if the file exists */
        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        /**
         * @brief Opens a file with the specified mode.
         * @param InPath The path to the file.
         * @param InMode The mode to open the file in (Read, Write, Append, etc.).
         * @return A unique pointer to the opened file, or an error code.
         */
        [[nodiscard]] static TExpected<TUniquePtr<FFile>, EErrorCode::Type> Open ( const FPath &InPath, EFileMode InMode ) noexcept;

        /**
         * @brief Reads data from the file into the provided buffer.
         * @param OutBuffer Pointer to the destination buffer.
         * @param InBytesToRead Number of bytes to read.
         * @return The actual number of bytes read, or an error code.
         */
        [[nodiscard]] TExpected<USize, EErrorCode::Type> Read ( void *OutBuffer, USize InBytesToRead ) noexcept;

        /**
         * @brief Writes data from the provided buffer to the file.
         * @param InBuffer Pointer to the source buffer.
         * @param InBytesToWrite Number of bytes to write.
         * @return The actual number of bytes written, or an error code.
         */
        [[nodiscard]] TExpected<USize, EErrorCode::Type> Write ( const void *InBuffer, USize InBytesToWrite ) noexcept;

        /**
         * @brief Sets the file position indicator.
         * @param InOffset The offset to move to.
         * @param InOrigin The origin to seek from (Begin, Current, End).
         * @return Success or failure.
         */
        TExpected<void, EErrorCode::Type> Seek ( Int64 InOffset, ESeekOrigin InOrigin ) noexcept;

        /**
         * @brief Gets the current file position.
         * @return The current byte offset, or an error code.
         */
        [[nodiscard]] TExpected<USize, EErrorCode::Type> Tell () const noexcept;

        /**
         * @brief Flushes any buffered data to the disk.
         */
        TExpected<void, EErrorCode::Type> Flush () noexcept;

        /**
         * @brief Closes the file handle. Called automatically on destruction.
         */
        void Close () noexcept;

    public:

        /**
         * @brief Copies a file, optionally reporting progress via a callback.
         * @param InSource The source file path.
         * @param InDestination The destination file path.
         * @param InOptions Copy behavior options.
         * @param InProgressCallback Optional callback to track progress (Done / Total).
         * @return Success or error code.
         */
        static TExpected<void, EErrorCode::Type>
        Copy ( const FPath &InSource, const FPath &InDestination, ECopyOptions InOptions = ECopyOptions::None, const FProgressCallback &InProgressCallback = nullptr );

        /**
         * @brief Moves or renames a file.
         */
        static TExpected<void, EErrorCode::Type> Move ( const FPath &InSource, const FPath &InDestination ) noexcept;

        /**
         * @brief Deletes a file.
         */
        static TExpected<void, EErrorCode::Type> Delete ( const FPath &InPath ) noexcept;

        /**
         * @brief Retrieves metadata information about a file.
         */
        [[nodiscard]] static TExpected<FFileInfo, EErrorCode::Type> GetInfo ( const FPath &InPath ) noexcept;

        /**
         * @brief Modifies the attributes of a file.
         */
        static TExpected<void, EErrorCode::Type> SetAttributes ( const FPath &InPath, EFileAttributes InAttributes ) noexcept;

    private:

        void *FileHandle = nullptr;
    };

} // namespace Filesystem

} // namespace LumenEngine