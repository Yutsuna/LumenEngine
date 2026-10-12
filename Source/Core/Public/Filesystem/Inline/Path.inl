///
/// @file CorPath.inl
/// @brief Inline implementation of FPath helpers.
///

#pragma once

#include <Filesystem/Path.hpp>

#include <algorithm>
#include <utility>

namespace Lumen
{

inline Filesystem::FPath::FPath ( const FStringView InPath ) noexcept : PathString( InPath )
{
    Normalize();
}

inline Filesystem::FPath::FPath ( FString InPath ) noexcept : PathString( std::move( InPath ) )
{
    Normalize();
}

inline Filesystem::FPath::FPath ( const AnsiChar *InPath ) noexcept : PathString( InPath ? InPath : "" )
{
    Normalize();
}

inline void Filesystem::FPath::Normalize () noexcept
{
    if ( PathString.empty() )
    {
        return;
    }

    std::replace( PathString.begin(), PathString.end(), '\\', '/' );

    const auto NewEnd = std::unique( PathString.begin(), PathString.end(),
                                     [] ( const AnsiChar A, const AnsiChar B ) { return A == '/' && B == '/'; } );
    PathString.erase( NewEnd, PathString.end() );

    if ( PathString.size() > 1 && PathString.back() == '/' )
    {
        PathString.pop_back();
    }
}

inline FStringView Filesystem::FPath::GetExtension () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    if ( DotPosition == FStringView::npos || DotPosition == 0 )
    {
        return {};
    }

    return Filename.substr( DotPosition + 1 );
}

inline FStringView Filesystem::FPath::GetFilename () const noexcept
{
    const USize SlashPosition = PathString.find_last_of( '/' );

    if ( SlashPosition == FString::npos )
    {
        return PathString;
    }

    return FStringView( PathString ).substr( SlashPosition + 1 );
}

inline FStringView Filesystem::FPath::GetStem () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    if ( DotPosition == FStringView::npos || DotPosition == 0 )
    {
        return Filename;
    }

    return Filename.substr( 0, DotPosition );
}

inline Filesystem::FPath Filesystem::FPath::GetParent () const noexcept
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
        return Filesystem::FPath( FStringView( PathString ).substr( 0, 1 ) );
    }

    return Filesystem::FPath( FStringView( PathString ).substr( 0, SlashPosition ) );
}

inline Bool Filesystem::FPath::HasExtension () const noexcept
{
    const FStringView Filename = GetFilename();
    const USize DotPosition    = Filename.find_last_of( '.' );

    return DotPosition != FStringView::npos && DotPosition > 0;
}

inline Bool Filesystem::FPath::IsRelative () const noexcept
{
    return !IsAbsolute();
}

inline Bool Filesystem::FPath::IsAbsolute () const noexcept
{
    return !PathString.empty() && PathString.front() == '/';
}

inline Bool Filesystem::FPath::IsEmpty () const noexcept
{
    return PathString.empty();
}

inline void Filesystem::FPath::Clear () noexcept
{
    PathString.clear();
}

inline Filesystem::FPath &Filesystem::FPath::Append ( FStringView InPart ) noexcept
{
    if ( PathString.empty() )
    {
        PathString = InPart;
    }
    else if ( !InPart.empty() )
    {
        PathString += '/';
        PathString += InPart;
    }

    Normalize();
    return *this;
}

inline Filesystem::FPath &Filesystem::FPath::operator/=( FStringView InPart ) noexcept
{
    return Append( InPart );
}

inline Bool Filesystem::FPath::operator==( const Filesystem::FPath &InOther ) const noexcept
{
    return PathString == InOther.PathString;
}

inline auto Filesystem::FPath::operator<=>( const Filesystem::FPath &InOther ) const noexcept
{
    return PathString <=> InOther.PathString;
}

inline const FString &Filesystem::FPath::ToString () const noexcept
{
    return PathString;
}

inline const AnsiChar *Filesystem::FPath::ToCString () const noexcept
{
    return PathString.c_str();
}

} // namespace Lumen