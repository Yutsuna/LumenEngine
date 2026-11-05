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

    ///
    /// @class FActorRegistry
    /// @brief Internal registry for mapping ActorIDs to AActor pointers.
    ///
    class FActorRegistry
    {
    private:

        /// @struct FActorIDHash
        /// @brief Hash function for ActorID keys in the registry.
        struct FActorIDHash
        {
            [[nodiscard]] USize operator()( const ActorID InId ) const noexcept;
        };

    public:

        /// @brief Registers an AActor with the registry.
        /// @param InActor The AActor to register.
        /// @return The result of the registration attempt.
        static EActorRegistrationResult Register ( AActor *InActor ) noexcept;

        /// @brief Unregisters an AActor from the registry.
        /// @param InActor The AActor to unregister.
        static void Unregister ( AActor *InActor ) noexcept;

        /// @brief Enqueues a message for an ActorID.
        /// @param InId The ActorID to enqueue the message for.
        /// @param InMessage The message to enqueue.
        /// @return The result of the enqueue attempt.
        static EEnqueueResult EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;

    private:

        struct FRegistryState
        {
            FSharedMutex RegistryMutex;
            TMap<ActorID, AActor *, FActorIDHash> Actors;
            TMap<UInt32, UInt32> Generations;
            UInt32 NextIndex = 1U;
        };

        [[nodiscard]] static FRegistryState &GetState () noexcept;
    };

} // namespace Internal

} // namespace Lumen
