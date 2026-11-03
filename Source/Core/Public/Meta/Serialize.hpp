/// @file Serialize.hpp
/// @brief Serialize reflected objects to string.

#include <Meta/Reflection.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <format>

namespace Lumen
{

namespace Meta
{

    /// @brief Formatted "{ TypeName { field=value, ... } }" of an aggregate.
    template <CReflected TObject> [[nodiscard]] FString SerializeToString ( const TObject &Object )
    {
#if LUMEN_HAS_REFLECTION
        FString Buffer;
        Buffer += std::format( "{} {{ ", TypeName<TObject>() );

        bool bFirst = true;
        ForEachMember( Object,
                       [&Buffer, &bFirst] ( const FStringView InName, const auto &InValue )
                       {
                           if ( not bFirst )
                           {
                               Buffer += ", ";
                           }
                           bFirst = false;
                           Buffer += std::format( "{}={}", InName, InValue );
                       } );
        Buffer += " }";

        return FString{ FStringView( Buffer ) };
#else
        static_cast<void>( Object );
        return FString{};
#endif
    }

} // namespace Meta

} // namespace Lumen
