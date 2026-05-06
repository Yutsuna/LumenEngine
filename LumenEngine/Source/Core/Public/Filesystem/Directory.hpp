/**
 * @file Directory.hpp
 * @brief High-level directory manipulation and traversal operations.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "Container/SharedPtr.hpp"
#include "Container/Vector.hpp"

#include "ErrorCodes.hpp"
#include "Filesystem/Path.hpp"
#include "Filesystem/Types.hpp"
#include "NonCopyable.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    /**
     * @class FDirectoryIterator
     * @brief Allows iterating through a directory
     */
    class LUMEN_ENGINE_API FDirectoryIterator final
    {
    public:

        using Pointer   = const FFileInfo *;
        using Reference = const FFileInfo &;

    public:

        FDirectoryIterator () noexcept = default;
        explicit FDirectoryIterator ( const FPath &InDir, bool bSkipPermissionErrors = true ) noexcept;
        ~FDirectoryIterator () noexcept;

        FDirectoryIterator ( const FDirectoryIterator &InOther ) noexcept;
        FDirectoryIterator &operator=( const FDirectoryIterator &InOther ) noexcept;

    public:

        [[nodiscard]] Reference operator*() const noexcept;
        [[nodiscard]] Pointer operator->() const noexcept;

        FDirectoryIterator &operator++() noexcept;

        [[nodiscard]] Bool operator==( const FDirectoryIterator &Other ) const noexcept;
        [[nodiscard]] Bool operator!=( const FDirectoryIterator &Other ) const noexcept;

        [[nodiscard]] FDirectoryIterator Begin () const noexcept;
        [[nodiscard]] FDirectoryIterator End () const noexcept;

    private:

        void Advance () noexcept;

    private:

        struct FInternalState;
        TSharedPtr<FInternalState> State;
        FFileInfo CurrentInfo;
        Bool bIsEnd = true;
    };

    /**
     * @class FDirectory
     * @brief Static utility class for directory operations.
     */
    class LUMEN_ENGINE_API FDirectory final
    {
    public:

        /**
         * @brief Checks if a directory exists.
         * @param InPath The path to the directory to check.
         * @return True if the directory exists, false otherwise.
         */
        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        /**
         * @brief Creates a directory.
         * @param InPath The path to the directory to create.
         * @return Success or error code.
         */
        static TExpected<void, EErrorCode::Type> Create ( const FPath &InPath ) noexcept;

        /**
         * @brief Creates a directory and any necessary parent directories.
         * @param InPath The path to the directory to create.
         * @return Success or error code.
         */
        static TExpected<void, EErrorCode::Type> CreateDirectories ( const FPath &InPath ) noexcept;

        /**
         * @brief Deletes a directory. If the directory is not empty, it will fail unless bInRecursive is true.
         * @param InPath The path to the directory to delete.
         * @param bInRecursive Whether to delete all contents recursively if the directory is not empty.
         * @return Success or error code.
         */
        static TExpected<void, EErrorCode::Type> Delete ( const FPath &InPath, Bool bInRecursive = false ) noexcept;

        /**
         * @brief Enumerates files in a directory matching an optional filter.
         * @param InPath The directory path to enumerate.
         * @param bInRecursive Whether to enumerate subdirectories recursively.
         * @param InFilter Optional filter predicate to select specific files.
         * @return A vector of file information, or an error code.
         */
        [[nodiscard]] static TExpected<TVector<FFileInfo>, EErrorCode::Type>
        GetFiles ( const FPath &InPath, Bool bInRecursive = false, const FFilterPredicate &InFilter = nullptr ) noexcept;

        /**
         * @brief Enumerates directories matching an optional filter.
         * @param InPath The directory path to enumerate.
         * @param bInRecursive Whether to enumerate subdirectories recursively.
         * @param InFilter Optional filter predicate to select specific directories.
         * @return A vector of directory information, or an error code.
         */
        [[nodiscard]] static TExpected<TVector<FFileInfo>, EErrorCode::Type>
        GetDirectories ( const FPath &InPath, Bool bInRecursive = false, const FFilterPredicate &InFilter = nullptr ) noexcept;
    };

} // namespace Filesystem

} // namespace LumenEngine