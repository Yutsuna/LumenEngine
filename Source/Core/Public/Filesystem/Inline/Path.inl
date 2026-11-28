/**
 * @file CorPath.inl
 * @brief Inline implementation of FPath helpers.
 */

#pragma once

#include "Core/CoreDefinitions.hpp"
#include <Filesystem/Path.hpp>

#include <utility>

/**
 * Public
 */

inline Lumen::Filesystem::FPath::FPath ( const FStringView InPath ) noexcept : PathString( InPath )
{
    Normalize();
}

inline Lumen::Filesystem::FPath::FPath ( FString InPath ) noexcept : PathString( std::move( InPath ) )
{
    Normalize();
}

inline Lumen::Filesystem::FPath::FPath ( const AnsiChar *InPath ) noexcept : PathString( ( InPath != nullptr ) ? InPath : "" )
{
    Normalize();
}

inline void Lumen::Filesystem::FPath::Normalize () noexcept
{
    if ( PathString.empty() )
    {
        return;
    }

    auto WriteIt = PathString.begin();
    Bool bSlash  = false;

    for ( const AnsiChar Character : PathString )
    {
        const Bool bNextSlash = Character == '\\' or Character == '/';
        if ( bNextSlash and bSlash )
        {
            continue;
        }

        *WriteIt = bNextSlash ? '/' : Character;
        ++WriteIt;
        bSlash = bNextSlash;
    }
    PathString.erase( WriteIt, PathString.end() );

    if ( PathString.size() > 1 and PathString.back() == '/' )
    {
        PathString.pop_back();
    }
}

inline Lumen::FStringView Lumen::Filesystem::FPath::GetExtension () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    if ( DotPosition == FStringView::npos or DotPosition == 0 )
    {
        return {};
    }

    return Filename.substr( DotPosition + 1 );
}

inline Lumen::FStringView Lumen::Filesystem::FPath::GetFilename () const noexcept
{
    const USize SlashPosition = PathString.find_last_of( '/' );

    if ( SlashPosition == FString::npos )
    {
        return PathString;
    }

    return FStringView( PathString ).substr( SlashPosition + 1 );
}

inline Lumen::FStringView Lumen::Filesystem::FPath::GetStem () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    if ( DotPosition == FStringView::npos or DotPosition == 0 )
    {
        return Filename;
    }

    return Filename.substr( 0, DotPosition );
}

inline Lumen::Filesystem::FPath Lumen::Filesystem::FPath::GetParent () const noexcept
{
    if ( PathString.empty() )
    {
        return {};
    }

    const USize SlashPosition = PathString.find_last_of( '/' );

    if ( SlashPosition == FString::npos )
    {
        return {};
    }

    if ( SlashPosition == 0 )
    {
        return Lumen::Filesystem::FPath( FStringView( PathString ).substr( 0, 1 ) );
    }

    return Lumen::Filesystem::FPath( FStringView( PathString ).substr( 0, SlashPosition ) );
}

inline Lumen::Bool Lumen::Filesystem::FPath::HasExtension () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    return DotPosition != FStringView::npos and DotPosition > 0;
}

inline Lumen::Bool Lumen::Filesystem::FPath::IsRelative () const noexcept
{
    return not IsAbsolute();
}

inline Lumen::Bool Lumen::Filesystem::FPath::IsAbsolute () const noexcept
{
    return not PathString.empty() and PathString.front() == '/';
}

inline Lumen::Bool Lumen::Filesystem::FPath::IsEmpty () const noexcept
{
    return PathString.empty();
}

inline void Lumen::Filesystem::FPath::Clear () noexcept
{
    PathString.clear();
}

inline Lumen::Filesystem::FPath &Lumen::Filesystem::FPath::Append ( FStringView InPart ) noexcept
{
    if ( PathString.empty() )
    {
        PathString = InPart;
    }
    else if ( not InPart.empty() )
    {
        PathString += '/';
        PathString += InPart;
    }

    Normalize();
    return *this;
}

inline Lumen::Filesystem::FPath &Lumen::Filesystem::FPath::operator/=( FStringView InPart ) noexcept
{
    return Append( InPart );
}

inline Lumen::Bool Lumen::Filesystem::FPath::operator==( const Filesystem::FPath &InOther ) const noexcept
{
    return PathString == InOther.PathString;
}

inline auto Lumen::Filesystem::FPath::operator<=>( const Filesystem::FPath &InOther ) const noexcept
{
    return PathString <=> InOther.PathString;
}

inline const Lumen::FString &Lumen::Filesystem::FPath::ToString () const noexcept
{
    return PathString;
}

inline const Lumen::AnsiChar *Lumen::Filesystem::FPath::ToCString () const noexcept
{
    return PathString.c_str();
}
