/// @file Directory.inl
/// @brief Inline directory traversal implementation.

#pragma once

#include <functional>
#include <type_traits>

namespace Lumen
{

namespace Filesystem
{

    namespace Internal
    {
        CORE_EXPORT TExpected<void, EFileError> PlatformEnumerateEntries (
            const FPath &InPath, Bool bRecursive, const std::function<Bool( const FPath &, Bool )> &InCallback );
    }

    template <typename FCallback>
    TExpected<void, EFileError> FDirectory::ForEachEntry ( const FPath &InPath, const Bool bRecursive, FCallback &&Callback )
    {
        return Internal::PlatformEnumerateEntries(
            InPath, bRecursive,
            [&Callback] ( const FPath &EntryPath, const Bool bIsDir ) -> Bool
            {
                if constexpr ( std::is_invocable_r_v<Bool, FCallback, const FPath &, Bool> )
                {
                    return Callback( EntryPath, bIsDir );
                }
                else
                {
                    Callback( EntryPath, bIsDir );
                    return true;
                }
            } );
    }

} // namespace Filesystem

} // namespace Lumen