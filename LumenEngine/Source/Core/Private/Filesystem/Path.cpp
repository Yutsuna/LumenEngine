/**
 * @file Path.cpp
 * @brief Implementation for the file system path class.
 */

#include "Filesystem/Path.hpp"
#include <filesystem>

/**
 * Ctor
 */

LumenEngine::Filesystem::FPath::FPath ( const FString &InPath ) : Path( InPath )
{
    /* */
}

LumenEngine::Filesystem::FPath::FPath ( const FStringView InPath ) : Path( InPath )
{
    /* */
}

LumenEngine::Filesystem::FPath::FPath ( const AnsiChar *const InPath ) : Path( InPath )
{
    /* */
}

/**
 * Public
 */

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsFile () const noexcept
{
    std::error_code EC;
    return std::filesystem::is_regular_file( Path, EC ) && !EC;
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsDirectory () const noexcept
{
    std::error_code EC;
    return std::filesystem::is_directory( Path, EC ) && !EC;
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsSymLink () const noexcept
{
    std::error_code EC;
    return std::filesystem::is_symlink( Path, EC ) && !EC;
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::Exists () const noexcept
{
    std::error_code EC;
    return std::filesystem::exists( Path, EC ) && !EC;
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsAbsolute () const noexcept
{
    return std::filesystem::path( Path ).is_absolute();
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsRelative () const noexcept
{
    return std::filesystem::path( Path ).is_relative();
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::IsEmpty () const noexcept
{
    return Path.empty();
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::HasExtension () const noexcept
{
    return std::filesystem::path( Path ).has_extension();
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::HasFilename () const noexcept
{
    return std::filesystem::path( Path ).has_filename();
}

LumenEngine::Bool LumenEngine::Filesystem::FPath::HasParentPath () const noexcept
{
    return std::filesystem::path( Path ).has_parent_path();
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::GetParentPath () const noexcept
{
    return FPath( std::filesystem::path( Path ).parent_path().string() );
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::GetAbsolutePath () const noexcept
{
    std::error_code EC;
    return FPath( std::filesystem::absolute( Path, EC ).string() );
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::GetRelativePath ( const LumenEngine::Filesystem::FPath &BasePath ) const noexcept
{
    std::error_code EC;
    return FPath( std::filesystem::relative( Path, BasePath.ToString(), EC ).string() );
}

LumenEngine::FString LumenEngine::Filesystem::FPath::GetFileName () const noexcept
{
    return std::filesystem::path( Path ).filename().string();
}

LumenEngine::FString LumenEngine::Filesystem::FPath::GetStem () const noexcept
{
    return std::filesystem::path( Path ).stem().string();
}

LumenEngine::FString LumenEngine::Filesystem::FPath::GetExtension () const noexcept
{
    return std::filesystem::path( Path ).extension().string();
}

LumenEngine::FString LumenEngine::Filesystem::FPath::ToString () const noexcept
{
    return Path;
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::operator/ ( const FPath &Other ) const noexcept
{
    std::filesystem::path P( Path );
    P /= Other.Path;
    return FPath( P.string() );
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::operator/ ( const FString &Other ) const noexcept
{
    std::filesystem::path P( Path );
    P /= Other;
    return FPath( P.string() );
}

LumenEngine::Filesystem::FPath LumenEngine::Filesystem::FPath::operator/ ( const AnsiChar *Other ) const noexcept
{
    std::filesystem::path P( Path );
    P /= Other;
    return FPath( P.string() );
}
