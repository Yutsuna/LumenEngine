/**
 * @file MemoryMappedFile.hpp
 * @brief Memory-mapped file support for high-performance zero-copy I/O.
 */

#pragma once

#include "Container/Expected.hpp"
#include "Container/Span.hpp"
#include "Container/UniquePtr.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "ErrorCodes.hpp"
#include "Filesystem/Path.hpp"
#include "NonCopyable.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    /**
     * @class FMemoryMappedFile
     * @brief Provides a zero-copy view of a file's contents using system memory mapping.
     */
    class LUMEN_ENGINE_API FMemoryMappedFile final : public FNonCopyable
    {
    public:

        FMemoryMappedFile () noexcept = default;
        ~FMemoryMappedFile () noexcept;

        FMemoryMappedFile ( FMemoryMappedFile &&InOther ) noexcept;
        FMemoryMappedFile &operator=( FMemoryMappedFile &&InOther ) noexcept;

    public:

        /**
         * @brief Maps a file into memory for reading.
         * @param InPath The path to the file.
         * @return A unique pointer to the mapped file, or an error code.
         */
        [[nodiscard]] static TExpected<TUniquePtr<FMemoryMappedFile>, EErrorCode::Type> Open ( const FPath &InPath ) noexcept;

    public:

        /** @return A span representing the entire mapped content. */
        [[nodiscard]] TSpan<const Byte> GetRegion () const noexcept;

        /** @return The size of the mapped region in bytes. */
        [[nodiscard]] USize GetSize () const noexcept;

        /** @return True if the file is successfully mapped. */
        [[nodiscard]] Bool IsMapped () const noexcept;

        /** @brief Unmaps the file. Called automatically on destruction. */
        void Close () noexcept;

    private:

        void *MappedData     = nullptr;
        USize MappedSize     = 0;
        Int32 FileDescriptor = -1;
    };

} // namespace Filesystem

} // namespace LumenEngine
