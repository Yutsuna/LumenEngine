/// @file Directory.hpp
/// @brief Filesystem Directory class abstraction.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>
#include <Filesystem/Path.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class FDirectory
    /// @brief High-level static utility facade for directories.
    ///
    class CORE_EXPORT FDirectory
    {
    public:

        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        static Bool Create ( const FPath &InPath ) noexcept;
        static Bool CreateDirectories ( const FPath &InPath ) noexcept;

        static Bool Delete ( const FPath &InPath, Bool bRecursive = false ) noexcept;

        ///
        /// @brief Enumerates the entries of a directory via a callback.
        /// @param InPath The directory path to scan.
        /// @param bRecursive Whether to scan recursively.
        /// @param Callback Callable taking `(const FPath &EntryPath, Bool bIsDirectory)`.
        ///        If Callback returns a Bool, returning false stops the iteration early.
        ///
        template <typename FCallback> static void ForEachEntry ( const FPath &InPath, Bool bRecursive, FCallback &&Callback );
    };

} // namespace Filesystem

} // namespace Lumen

#include <Filesystem/Inline/Directory.inl>
