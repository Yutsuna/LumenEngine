/// @file Path.hpp
/// @brief Filesystem definition of the FPath class.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

namespace Lumen
{

namespace Filesystem
{

    ///
    /// @class FPath
    /// @brief Represents a normalized filesystem path.
    ///
    class CORE_EXPORT FPath
    {
    public:

        FPath () noexcept = default;
        explicit FPath ( FStringView InPath ) noexcept;
        explicit FPath ( FString InPath ) noexcept;
        explicit FPath ( const AnsiChar *InPath ) noexcept;

    public:

        [[nodiscard]] FStringView GetExtension () const noexcept;
        [[nodiscard]] FStringView GetFilename () const noexcept;
        [[nodiscard]] FStringView GetStem () const noexcept;
        [[nodiscard]] FPath GetParent () const noexcept;

        [[nodiscard]] Bool HasExtension () const noexcept;
        [[nodiscard]] Bool IsRelative () const noexcept;
        [[nodiscard]] Bool IsAbsolute () const noexcept;
        [[nodiscard]] Bool IsEmpty () const noexcept;

        void Clear () noexcept;

    public:

        FPath &Append ( FStringView InPart ) noexcept;
        FPath &operator/=( FStringView InPart ) noexcept;

        [[nodiscard]] friend FPath operator/( FPath InLeft, const FStringView InRight ) noexcept
        {
            InLeft.Append( InRight );
            return InLeft;
        }

    public:

        [[nodiscard]] Bool operator==( const FPath &InOther ) const noexcept;
        [[nodiscard]] auto operator<=>( const FPath &InOther ) const noexcept;

        [[nodiscard]] const FString &ToString () const noexcept;
        [[nodiscard]] const AnsiChar *ToCString () const noexcept;

    private:

        void Normalize () noexcept;

    private:

        FString PathString;
    };

} // namespace Filesystem

} // namespace Lumen

#include <Filesystem/Inline/Path.inl>