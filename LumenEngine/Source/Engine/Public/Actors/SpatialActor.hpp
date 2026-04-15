/**
 * @file SpatialActor.hpp
 * @brief Declaration of the ASpatialActor class
 */

#pragma once

#include "Actor/Actor.hpp"
#include "Actor/ActorTypes.hpp"

namespace LumenEngine
{

/**
 * @class ASpatialActor
 * @brief An Actor that exists in the spatial environment of the game world.
 */
class LUMEN_ENGINE_API ASpatialActor : public AActor
{
public:

    explicit ASpatialActor ( ActorID InId ) noexcept;
    ~ASpatialActor () noexcept override;
};

} // namespace LumenEngine
