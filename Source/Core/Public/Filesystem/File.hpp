/// @file File.hpp
/// @brief Static helper facade for file operations.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <Array/Vector.hpp>
#include <Memory/Span.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>

#include <Templates/Expected.hpp>

#include <Filesystem/FileTypes.hpp>
#include <Filesystem/Path.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class FFile
    /// @brief High-level static utility facade for file I/O.
    ///
    class CORE_EXPORT FFile
    {
    public:

        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        [[nodiscard]] static TExpected<FString, EFileError> ReadAllText ( const FPath &InPath );
        [[nodiscard]] static TExpected<TVector<Byte>, EFileError> ReadAllBytes ( const FPath &InPath );

        static TExpected<void, EFileError> WriteAllText ( const FPath &InPath, FStringView InContent ) noexcept;
        static TExpected<void, EFileError> WriteAllBytes ( const FPath &InPath, TSpan<const Byte> InData ) noexcept;

        static Bool Delete ( const FPath &InPath ) noexcept;
        static Bool Copy ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept;
        static Bool Move ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept;

        [[nodiscard]] static TExpected<UInt64, EFileError> GetSize ( const FPath &InPath ) noexcept;
    };

} // namespace Filesystem

} // namespace Lumen