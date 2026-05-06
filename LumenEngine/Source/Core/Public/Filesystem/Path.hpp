/**
 * @file Path.hpp
 * @brief This file contains the definition for the file system path class
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/String.hpp"

namespace LumenEngine
{

namespace Filesystem
{

    class FPath final
    {
    public:

        FPath () noexcept = default;

        FPath ( const FString &InPath );

        FPath ( const FStringView InPath );

        FPath ( const AnsiChar *const InPath );

    public:

        [[nodiscard]] Bool IsFile () const noexcept;

        [[nodiscard]] Bool IsDirectory () const noexcept;

        [[nodiscard]] Bool IsSymLink () const noexcept;

        [[nodiscard]] Bool Exists () const noexcept;

        [[nodiscard]] Bool IsAbsolute () const noexcept;

        [[nodiscard]] Bool IsRelative () const noexcept;

        [[nodiscard]] Bool IsEmpty () const noexcept;

        [[nodiscard]] Bool HasExtension () const noexcept;

        [[nodiscard]] Bool HasFilename () const noexcept;

        [[nodiscard]] Bool HasParentPath () const noexcept;

    public:

        [[nodiscard]] FPath GetParentPath () const noexcept;

        [[nodiscard]] FPath GetAbsolutePath () const noexcept;

        [[nodiscard]] FPath GetRelativePath ( const FPath &BasePath ) const noexcept;

        [[nodiscard]] FString GetFileName () const noexcept;

        [[nodiscard]] FString GetStem () const noexcept;

        [[nodiscard]] FString GetExtension () const noexcept;

        [[nodiscard]] FString ToString () const noexcept;

    public:

        FPath operator/ ( const FPath &Other ) const noexcept;
        FPath operator/ ( const FString &Other ) const noexcept;
        FPath operator/ ( const AnsiChar *Other ) const noexcept;

    private:

        FString Path;
    };

} // namespace Filesystem

} // namespace LumenEngine
