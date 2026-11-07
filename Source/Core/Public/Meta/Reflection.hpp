/**
 * @file Reflection.hpp
 * @brief C++26 static reflection helpers over aggregates.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <String/String.hpp>

#include <Concepts/SameAs.hpp>

#include <string_view>
#include <type_traits>
#include <variant>

#if __has_include( <meta> ) && defined( __cpp_expansion_statements )
    #include <meta>
    #define LUMEN_HAS_REFLECTION 1
#else
    #define LUMEN_HAS_REFLECTION 0
#endif

namespace Lumen
{

namespace Meta
{

    namespace Internal
    {
        /** @brief The one field name reflection skips: source-location plumbing. */
        inline constexpr std::string_view HiddenField = "Loc";
    } // namespace Internal

    /** @brief A type is Reflected when field iteration can walk it: an aggregate class. */
    template <typename TObject>
    concept CReflected = std::is_class_v<std::remove_cvref_t<TObject>> && std::is_aggregate_v<std::remove_cvref_t<TObject>>;

    /** @brief Invoke Callback( Name, FieldRef ) for each semantic field, in declaration order. */
    template <CReflected TObject, typename TFunc> constexpr void ForEachMember ( TObject &Object, TFunc &&Callback )
    {
#if LUMEN_HAS_REFLECTION
        using Bare = std::remove_cvref_t<TObject>;
        static constexpr auto Members =
            std::define_static_array( std::meta::nonstatic_data_members_of( ^^Bare, std::meta::access_context::unchecked() ) );

        template for ( constexpr auto Member : Members )
        {
            if constexpr ( std::meta::identifier_of( Member ) != Internal::HiddenField )
            {
                Callback( std::define_static_string( std::meta::identifier_of( Member ) ), Object.[:Member:] );
            }
        }
#else
        static_cast<void>( Object );
        static_cast<void>( Callback );
#endif
    }

    /** @brief Number of semantic fields ForEachMember will visit. */
    template <CReflected TObject> [[nodiscard]] consteval USize MemberCount ()
    {
#if LUMEN_HAS_REFLECTION
        USize Count = 0;
        for ( const auto Member :
              std::meta::nonstatic_data_members_of( ^^std::remove_cvref_t<TObject>, std::meta::access_context::unchecked() ) )
        {
            if ( std::meta::identifier_of( Member ) != Internal::HiddenField )
            {
                ++Count;
            }
        }
        return Count;
#else
        return 0;
#endif
    }

    /** @brief Enumerator name of Value, or "<unknown>" for out-of-range values. */
    template <typename TEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr std::string_view EnumName ( TEnum Value )
    {
#if LUMEN_HAS_REFLECTION
        template for ( constexpr auto Enumerator : std::define_static_array( std::meta::enumerators_of( ^^TEnum ) ) )
        {
            if ( Value == [:Enumerator:] )
            {
                return std::meta::identifier_of( Enumerator );
            }
        }
        return "<unknown>";
#else
        static_cast<void>( Value );
        return "<unknown>";
#endif
    }

    /** @brief Unqualified type name of T ("FLaunchEngineConfig", "FString"), aliases resolved. */
    template <typename TType> [[nodiscard]] consteval std::string_view TypeName ()
    {
#if LUMEN_HAS_REFLECTION
        constexpr std::meta::info Type = std::meta::dealias( ^^std::remove_cvref_t<TType> );
        constexpr std::string_view Name =
            std::meta::has_identifier( Type ) ? std::meta::identifier_of( Type ) : std::meta::display_string_of( Type );
        return Name;
#else
        return "Unknown";
#endif
    }

    /** @brief Type name of the active alternative of a variant (monostate → "None"). */
    template <typename... Alts> [[nodiscard]] constexpr std::string_view ActiveName ( const std::variant<Alts...> &Node )
    {
#if LUMEN_HAS_REFLECTION
        return std::visit(
            [] ( const auto &Alt ) -> std::string_view
            {
                using TObject = std::remove_cvref_t<decltype( Alt )>;
                if constexpr ( Concepts::CSameAs<TObject, std::monostate> )
                {
                    return "None";
                }
                else
                {
                    return TypeName<TObject>();
                }
            },
            Node );
#else
        static_cast<void>( Node );
        return "Unknown";
#endif
    }

} // namespace Meta

} // namespace Lumen
