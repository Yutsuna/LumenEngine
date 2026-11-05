/// @file SerializeJSON.hpp
/// @brief Serialize reflected objects to JSON.

#pragma once

#include <Meta/Reflection.hpp>

#include <Core/CoreTypes.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <Templates/Optional.hpp>

#include <Concepts/SameAs.hpp>

namespace Lumen
{

namespace Meta
{

    /// @brief Options for SerializeToJSON.
    struct FJSONOptions
    {
        /// @brief Emit newlines + indentation instead of a compact single line.
        Bool bPretty = true;

        /// @brief Number of spaces per indentation level (pretty mode only).
        USize IndentSize = 4;
    };

    namespace Detail
    {

        /// @brief Newline + indentation when pretty, nothing otherwise.
        inline void WriteBreak ( FString &Out, const FJSONOptions &Options, const USize Depth );

        /// @brief Write Text as a quoted, escaped JSON string.
        inline void WriteJSONString ( FString &Out, const FStringView Text );

        /// @brief Write one value, picking the JSON shape from the type.
        template <typename TValue>
        void WriteJSONValue ( FString &Out, const TValue &Value, const FJSONOptions &Options, USize Depth );

        /// @brief Write `"Name": Value`.
        template <typename TValue>
        void WriteJSONMember (
            FString &Out, const FStringView Name, const TValue &Value, const FJSONOptions &Options, const USize Depth );

    } // namespace Detail

    /// @brief Serialize a reflected aggregate to JSON.
    /// @details bool -> true/false, numbers -> numbers (NaN/Inf -> null), enums -> enumerator name,
    ///          strings -> escaped strings, source_location -> "file:line", ranges -> arrays,
    ///          TOptional/monostate -> null when empty, variant -> { "Type", "Value" },
    ///          nested aggregates -> objects.
    template <CReflected TObject>
    [[nodiscard]] FString SerializeToJSON ( const TObject &Object, const FJSONOptions &Options = {} )
    {
#if LUMEN_HAS_REFLECTION
        FString Out;
        Detail::WriteJSONValue( Out, Object, Options, 0 );
        return Out;
#else
        static_cast<void>( Object );
        static_cast<void>( Options );
        return FString{};
#endif
    }

} // namespace Meta

} // namespace Lumen

#include "Inline/SerializeToJSON.inl"
