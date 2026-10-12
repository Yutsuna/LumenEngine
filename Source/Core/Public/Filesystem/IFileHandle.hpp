/// @file IFileHandle.hpp
/// @brief Interface for low-level raw byte file handle.

#pragma once

#include <Core/CoreTypes.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Memory/Span.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class IFileHandle
    /// @brief Low-level stream/handle interface for reading and writing raw bytes.
    ///
    class CORE_EXPORT IFileHandle
    {
    public:

        virtual ~IFileHandle () noexcept = default;

    public:

        /// @brief Reads bytes into the provided span.
        /// @return Number of bytes actually read.
        virtual USize Read ( TSpan<Byte> OutBuffer ) noexcept = 0;

        /// @brief Writes bytes from the provided span.
        /// @return Number of bytes actually written.
        virtual USize Write ( TSpan<const Byte> InBuffer ) noexcept = 0;

        /// @brief Moves the cursor position in the file.
        virtual Bool Seek ( Int64 Offset, ESeekOrigin Origin ) noexcept = 0;

        /// @brief Current cursor position.
        [[nodiscard]] virtual Int64 Tell () const noexcept = 0;

        /// @brief Total file size.
        [[nodiscard]] virtual UInt64 Size () const noexcept = 0;

        /// @brief Flushes pending writes to disk (fsync / FlushFileBuffers).
        /// @return True if flush was successful.
        virtual Bool Flush () noexcept = 0;

        /// @brief Checks if the cursor is at or past the end of file.
        [[nodiscard]] virtual Bool IsEOF () const noexcept
        {
            return Tell() >= static_cast<Int64>( Size() );
        }

    public:

        /// @brief Maps the entire file into memory for zero-copy read access.
        /// @return Span over mapped memory, or empty span if unsupported.
        [[nodiscard]] virtual TSpan<const Byte> Map () noexcept
        {
            return {};
        }

        /// @brief Unmaps a previously mapped file region.
        virtual void Unmap () noexcept
        {
            /// Empty
        }
    };

} // namespace Filesystem

} // namespace Lumen