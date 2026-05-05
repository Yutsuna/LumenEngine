/**
 * @file Path.cpp
 * @brief This file contains the implementation for the file system path class
 */

#include "Filesystem/Path.hpp"

#include "CoreTypes.hpp"

/**
 * Ctor
 */

LumenEngine::Filesystem::FPath::FPath ( FString &InPath ) : Path( ( InPath ) )
{
    /**/
}

LumenEngine::Filesystem::FPath::FPath ( const FStringView InPath ) : Path( InPath )
{
    /**/
}

LumenEngine::Filesystem::FPath::FPath ( const AnsiChar *const InPath ) : Path( InPath )
{
    /**/
}

/**
 * Public
 */

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsFile () const noexcept
{
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsDirectory () const noexcept
{
}