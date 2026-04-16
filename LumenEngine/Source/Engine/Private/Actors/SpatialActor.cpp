/**
 * @file SpatialActor.cpp
 * @brief Implementation of the ASpatialActor class
 */

#include "Actors/SpatialActor.hpp"
#include "World/SpatialRegistry.hpp"

LumenEngine::ASpatialActor::ASpatialActor ( ActorID InId ) noexcept : AActor( InId )
{
    Engine::FSpatialRegistry::Get().RegisterSpatialEntity( InId );
}

LumenEngine::ASpatialActor::~ASpatialActor () noexcept
{
    Engine::FSpatialRegistry::Get().UnregisterSpatialEntity( GetId() );
}
