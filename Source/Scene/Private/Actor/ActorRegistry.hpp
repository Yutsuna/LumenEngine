///
/// @file ActorRegistry.hpp
/// @brief Internal registry for mapping ActorIDs to AActor pointers.
///

#pragma once

#include <Actor/ActorTypes.hpp>
#include <HAL/SharedMutex.hpp>
#include <Templates/Map.hpp>

namespace Lumen
{

namespace Internal
{

    class FActorRegistry
    {
    private:

        struct FActorIDHash
        {
            [[nodiscard]] USize operator()( const ActorID InId ) const noexcept
            {
                const UInt64 Key = ( static_cast<UInt64>( InId.Generation ) << 32U ) | InId.Index;
                return static_cast<USize>( Key ^ ( Key >> 32U ) );
            }
        };

    public:

        static EActorRegistrationResult Register ( AActor *InActor ) noexcept;
        static void Unregister ( AActor *InActor ) noexcept;
        static EEnqueueResult EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;

    private:

        static FSharedMutex RegistryMutex;
        static TMap<ActorID, AActor *, FActorIDHash> Actors;
        static TMap<UInt32, UInt32> Generations;
        static UInt32 NextIndex;
    };

} // namespace Internal

} // namespace Lumen
