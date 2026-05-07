/**
 * @file Types.cpp
 * @brief Implementation of file system types
 */

#include "Filesystem/Types.hpp"
#include "CoreTypes.hpp"

LumenEngine::Bool LumenEngine::Filesystem::FFileInfo::IsDirectory () const noexcept
{
    return ( Attributes & EFileAttributes::Directory ) != EFileAttributes::None;
}

LumenEngine::Bool LumenEngine::Filesystem::FFileInfo::IsReadOnly () const noexcept
{
    return ( Attributes & EFileAttributes::ReadOnly ) != EFileAttributes::None;
}

LumenEngine::Bool LumenEngine::Filesystem::FFileInfo::IsSymLink () const noexcept
{
    return ( Attributes & EFileAttributes::SymLink ) != EFileAttributes::None;
}