/**
 * @file TriangleActor.hpp
 * @brief Declaration of the ATriangleActor class
 */

#pragma once

#include "Actors/StaticMeshActor.hpp"

namespace LumenEngine
{

/**
 * @class ATriangleMeshActor
 * @brief Static mesh actor for the TriangleExample
 */
class ATriangleMeshActor final : public Engine::AStaticMeshActor
{
public:

    ATriangleMeshActor ( ActorID InId ) noexcept;
};

} // namespace LumenEngine