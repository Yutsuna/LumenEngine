///
/// @file Directory.inl
/// @brief Inline implementation of FDirectory helpers.
///

#pragma once

#include <Filesystem/Directory.hpp>

#include <dirent.h>
#include <sys/stat.h>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Filesystem
{

    namespace Detail
    {

        template <typename FCallback, typename = void> struct TDirectoryCallbackReturnsBool : std::false_type
        {
            /// Empty
        };

        template <typename FCallback>
        struct TDirectoryCallbackReturnsBool<FCallback, std::void_t<std::invoke_result_t<FCallback, const FPath &, Bool>>>
            : std::is_same<std::invoke_result_t<FCallback, const FPath &, Bool>, Bool>
        {
            /// Empty
        };

        template <typename FCallback>
        constexpr Bool bDirectoryCallbackReturnsBool = TDirectoryCallbackReturnsBool<FCallback>::value;

    } // namespace Detail

    template <typename FCallback> void FDirectory::ForEachEntry ( const FPath &InPath, Bool bRecursive, FCallback &&Callback )
    {
        DIR *const DirectoryStream = opendir( InPath.ToCString() );
        if ( DirectoryStream == nullptr )
        {
            return;
        }

        while ( const dirent *const Entry = readdir( DirectoryStream ) )
        {
            const AnsiChar *const Name = Entry->d_name;

            if ( Name[0] == '.' && ( Name[1] == '\0' || ( Name[1] == '.' && Name[2] == '\0' ) ) )
            {
                continue;
            }

            FPath EntryPath = InPath;
            EntryPath /= FStringView( Name );

            struct stat StatBuffer{};
            const Bool bIsDirectory = stat( EntryPath.ToCString(), &StatBuffer ) == 0 && S_ISDIR( StatBuffer.st_mode );

            if constexpr ( Detail::bDirectoryCallbackReturnsBool<FCallback> )
            {
                if ( !Callback( EntryPath, bIsDirectory ) )
                {
                    closedir( DirectoryStream );
                    return;
                }
            }
            else
            {
                Callback( EntryPath, bIsDirectory );
            }

            if ( bRecursive && bIsDirectory )
            {
                ForEachEntry( EntryPath, bRecursive, std::forward<FCallback>( Callback ) );
            }
        }

        closedir( DirectoryStream );
    }

} // namespace Filesystem

} // namespace Lumen