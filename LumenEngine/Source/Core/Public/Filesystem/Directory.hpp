/**
 * @file Directory.hpp
 * @brief Contains the definition for the file system directory class
 */

#pragma once

#include "Filesystem/Path.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    /**
     * @class FDirectoryIterator
     * @brief An iterator for traversing the contents of a directory
     */
    class FDirectoryIterator final
    {

    public:

        using Pointer   = const FFileInfo *;
        using Reference = const FFileInfo &;

    public:

        FDirectoryIterator () noexcept = default;

        explicit FDirectoryIterator ( const FPath &InDir, bool bSkipPermissionErrors = true ) noexcept;

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
    };

    /**
     * @class FDirectoryRecursiveIterator
     * @brief An iterator for recursively traversing the contents of a directory
     */
    class FDirectoryRecursiveIterator final
    {
    public:

        using Pointer   = const FFileInfo *;
        using Reference = const FFileInfo &;

    public:

        FDirectoryRecursiveIterator () noexcept = default;

        explicit FDirectoryRecursiveIterator ( const FPath &InDir, bool bSkipPermissionErrors = true ) noexcept;

    public:

        [[nodiscard]] Reference operator*() const noexcept;
        [[nodiscard]] Pointer operator->() const noexcept;

        FDirectoryRecursiveIterator &operator++() noexcept;

        [[nodiscard]] Bool operator==( const FDirectoryRecursiveIterator &Other ) const noexcept;
        [[nodiscard]] Bool operator!=( const FDirectoryRecursiveIterator &Other ) const noexcept;

        [[nodiscard]] FDirectoryRecursiveIterator Begin () const noexcept;
        [[nodiscard]] FDirectoryRecursiveIterator End () const noexcept;

    private:

        void Advance () noexcept;

    private:
    };

} // namespace Filesystem

} // namespace LumenEngine