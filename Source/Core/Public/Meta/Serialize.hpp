/// @file Serialize.hpp
/// @brief Serialize reflected objects to string.

#include <Meta/Reflection.hpp>

#include <Core/CoreDefinitions.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <format>

namespace Lumen
{

namespace Meta
{

    namespace Detail
    {

        /// @brief Write one field value to Out, picking the format from the type.
        template <typename TOut, typename TValue> TOut FormatField ( TOut Out, const TValue &Value )
        {
            using Bare = std::remove_cvref_t<TValue>;

            if constexpr ( std::is_floating_point_v<Bare> )
            {
                return std::format_to( Out, "{:.6g}", Value );
            }
            else if constexpr ( std::is_enum_v<Bare> )
            {
                const FStringView Name = EnumName( Value );
                if ( Name == "<unknown>" )
                {
                    return std::format_to( Out, "{}", static_cast<std::underlying_type_t<Bare>>( Value ) );
                }
                return std::format_to( Out, "{}", Name );
            }
            else
            {
                return std::format_to( Out, "{}", Value );
            }
        }

    } // namespace Detail

    template <CReflected TObject> [[nodiscard]] FString SerializeToString ( const TObject &Object )
    {
#if LUMEN_HAS_REFLECTION
        return std::format( "{}", Object );
#else
        static_cast<void>( Object );
        return FString{};
#endif
    }

} // namespace Meta

} // namespace Lumen

// NOLINTBEGIN(cert-dcl58-cpp, bugprone-std-namespace-modification, readability-identifier-naming)
namespace std
{

/// @brief Formatter for any reflected aggregate.
template <Lumen::Meta::CReflected TObject> struct formatter<TObject, char>
{
    constexpr auto parse ( format_parse_context &Ctx )
    {
        return Ctx.begin();
    }

    template <typename FormatContext> auto format ( const TObject &Object, FormatContext &Ctx ) const
    {
        auto Out = Ctx.out();
        Out      = std::format_to( Out, "{}" LUMEN_NEWLINE "{{", Lumen::Meta::TypeName<TObject>() );

        Lumen::Bool bFirst = true;
        Lumen::Meta::ForEachMember( Object,
                                    [&Out, &bFirst] ( const Lumen::FStringView InName, const auto &InValue )
                                    {
                                        Out    = std::format_to( Out,
                                                                 "{}{}=", bFirst ? LUMEN_NEWLINE "    " : "," LUMEN_NEWLINE "    ",
                                                                 Lumen::FStringView( InName ) );
                                        Out    = Lumen::Meta::Detail::FormatField( Out, InValue );
                                        bFirst = false;
                                    } );

        return std::format_to( Out, LUMEN_NEWLINE "}}" );
    }
};

} // namespace std
// NOLINTEND(cert-dcl58-cpp, bugprone-std-namespace-modification, readability-identifier-naming)
