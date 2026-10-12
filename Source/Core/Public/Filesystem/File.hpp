/// @file File.hpp
/// @brief Static helper facade for file operations.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>
#include <Filesystem/FileTypes.hpp>
#include <Filesystem/Path.hpp>
#include <Memory/Span.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class FFile
    /// @brief High-level monadic static utility facade for file I/O.
    ///
    class CORE_EXPORT FFile
    {
    public:

        [[nodiscard]] static Bool Exists ( const FPath &InPath ) noexcept;

        [[nodiscard]] static TExpected<FString, EFileError> ReadAllText ( const FPath &InPath );
        [[nodiscard]] static TExpected<TVector<Byte>, EFileError> ReadAllBytes ( const FPath &InPath );

        static TExpected<void, EFileError> WriteAllText ( const FPath &InPath, FStringView InContent ) noexcept;
        static TExpected<void, EFileError> WriteAllBytes ( const FPath &InPath, TSpan<const Byte> InData ) noexcept;

        static TExpected<void, EFileError> Delete ( const FPath &InPath ) noexcept;
        static TExpected<void, EFileError>
        Copy ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept;
        static TExpected<void, EFileError>
        Move ( const FPath &InSource, const FPath &InDestination, Bool bOverwrite = true ) noexcept;

        [[nodiscard]] static TExpected<UInt64, EFileError> GetSize ( const FPath &InPath ) noexcept;
    };

} // namespace Filesystem

} // namespace Lumen