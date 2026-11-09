/**
 * @file SerializeToJSON.inl
 * @brief Inline implementations for SerializeToJSON.
 */

#pragma once

#include <Concepts/IsStringLike.hpp>
#include <Meta/SerializeToJSON.hpp>
#include <Templates/Variant.hpp>

#include <cmath>
#include <format>
#include <source_location>

inline void Lumen::Meta::Detail::WriteBreak ( FString &Out, const FJSONOptions &Options, const USize Depth )
{
    if ( not Options.bPretty )
    {
        return;
    }

    Out.push_back( '\n' );
    Out.append( Depth * Options.IndentSize, ' ' );
}

inline void Lumen::Meta::Detail::WriteJSONString ( FString &Out, const FStringView Text )
{
    Out.push_back( '"' );
    for ( const AnsiChar Char : Text )
    {
        switch ( Char )
        {
        case '"':
            Out.append( "\\\"" );
            break;
        case '\\':
            Out.append( "\\\\" );
            break;
        case '\b':
            Out.append( "\\b" );
            break;
        case '\f':
            Out.append( "\\f" );
            break;
        case '\n':
            Out.append( "\\n" );
            break;
        case '\r':
            Out.append( "\\r" );
            break;
        case '\t':
            Out.append( "\\t" );
            break;
        default:
            if ( static_cast<Byte>( Char ) < 0x20 )
            {
                std::format_to( std::back_inserter( Out ), "\\u{:04x}", static_cast<Byte>( Char ) );
            }
            else
            {
                Out.push_back( Char );
            }
            break;
        }
    }
    Out.push_back( '"' );
}

template <typename TValue>
void Lumen::Meta::Detail::WriteJSONValue ( FString &Out, const TValue &Value, const FJSONOptions &Options, const USize Depth )
{
    using Bare = std::remove_cvref_t<TValue>;

    if constexpr ( Concepts::CSameAs<Bare, Bool> )
    {
        Out.append( Value ? "true" : "false" );
    }
    else if constexpr ( std::is_floating_point_v<Bare> )
    {
        if ( std::isfinite( Value ) )
        {
            std::format_to( std::back_inserter( Out ), "{}", Value ); // shortest round-trip
        }
        else
        {
            Out.append( "null" );
        }
    }
    else if constexpr ( std::is_integral_v<Bare> )
    {
        std::format_to( std::back_inserter( Out ), "{}", Value );
    }
    else if constexpr ( std::is_enum_v<Bare> )
    {
        const FStringView Name = EnumName( Value );
        if ( Name == "<unknown>" )
        {
            std::format_to( std::back_inserter( Out ), "{}", static_cast<std::underlying_type_t<Bare>>( Value ) );
        }
        else
        {
            WriteJSONString( Out, Name );
        }
    }
    else if constexpr ( Concepts::CIsStringLike<Bare> )
    {
        WriteJSONString( Out, static_cast<FStringView>( Value ) );
    }
    else if constexpr ( Concepts::CSameAs<Bare, std::source_location> )
    {
        WriteJSONString( Out, std::format( "{}:{}", Value.file_name(), Value.line() ) );
    }
    else if constexpr ( Concepts::CSameAs<Bare, std::monostate> )
    {
        Out.append( "null" );
    }
    else if constexpr ( IsOptionalV<Bare> )
    {
        if ( Value.HasValue() )
        {
            WriteJSONValue( Out, *Value, Options, Depth );
        }
        else
        {
            Out.append( "null" );
        }
    }
    else if constexpr ( IsVariantV<Bare> )
    {
        if ( std::holds_alternative<std::monostate>( Value ) )
        {
            Out.append( "null" );
        }
        else
        {
            Out.push_back( '{' );
            WriteBreak( Out, Options, Depth + 1 );
            WriteJSONMember( Out, "Type", ActiveName( Value ), Options, Depth + 1 );
            Out.push_back( ',' );
            WriteBreak( Out, Options, Depth + 1 );
            Out.append( "\"Value\"" );
            Out.append( Options.bPretty ? ": " : ":" );
            std::visit( [&] ( const auto &Alt )
            {
                WriteJSONValue( Out, Alt, Options, Depth + 1 );
            }, Value );
            WriteBreak( Out, Options, Depth );
            Out.push_back( '}' );
        }
    }
    else if constexpr ( std::ranges::input_range<const Bare> )
    {
        Out.push_back( '[' );
        Bool bFirst = true;
        for ( const auto &Element : Value )
        {
            if ( not bFirst )
            {
                Out.push_back( ',' );
            }
            WriteBreak( Out, Options, Depth + 1 );
            WriteJSONValue( Out, Element, Options, Depth + 1 );
            bFirst = false;
        }
        if ( not bFirst )
        {
            WriteBreak( Out, Options, Depth );
        }
        Out.push_back( ']' );
    }
    else if constexpr ( CReflected<Bare> )
    {
        Out.push_back( '{' );
        Bool bFirst = true;
        ForEachMember( Value, [&] ( const FStringView Name, const auto &Field )
        {
            if ( not bFirst )
            {
                Out.push_back( ',' );
            }
            WriteBreak( Out, Options, Depth + 1 );
            WriteJSONMember( Out, Name, Field, Options, Depth + 1 );
            bFirst = false;
        } );
        if ( not bFirst )
        {
            WriteBreak( Out, Options, Depth );
        }
        Out.push_back( '}' );
    }
    else
    {
        static_assert( AlwaysFalseV<Bare>, "SerializeToJSON: unsupported field type" );
    }
}

template <typename TValue>
void Lumen::Meta::Detail::WriteJSONMember (
    FString &Out, const FStringView Name, const TValue &Value, const FJSONOptions &Options, const USize Depth )
{
    WriteJSONString( Out, Name );
    Out.append( Options.bPretty ? ": " : ":" );
    WriteJSONValue( Out, Value, Options, Depth );
}
