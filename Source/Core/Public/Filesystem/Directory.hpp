/// @file Directory.hpp
/// @brief Filesystem Directory class abstraction.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Filesystem/Path.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class FDirectory
    /// @brief High-level monadic static utility facade for directories.
    ///
    class CORE_EXPORT FDirectory
    {
    public:

        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        static TExpected<void, EFileError> Create ( const FPath &InPath ) noexcept;
        static TExpected<void, EFileError> CreateDirectories ( const FPath &InPath ) noexcept;

        static TExpected<void, EFileError> Delete ( const FPath &InPath, Bool bRecursive = false ) noexcept;

        ///
        /// @brief Enumerates the entries of a directory via a callback.
        /// @param Callback Callable taking `(const FPath &EntryPath, Bool bIsDirectory)`.
        ///        If Callback returns a Bool, returning false stops the iteration early.
        ///
        template <typename FCallback>
        static TExpected<void, EFileError> ForEachEntry ( const FPath &InPath, Bool bRecursive, FCallback &&Callback );
    };

} // namespace Filesystem

} // namespace Lumen

#include <Filesystem/Inline/Directory.inl>