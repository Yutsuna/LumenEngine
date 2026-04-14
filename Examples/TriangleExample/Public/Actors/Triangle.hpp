/**
 * @file Triangle.hpp
 * @brief Declaration of the ATriangle class.
 */

#pragma once

#include "Actors/StaticMeshActor.hpp"

namespace LumenEngine
{

/**
 * @class ATriangle
 * @brief An actor representing a 3D mesh in the scene.
 */
class ATriangle : public Engine::AStaticMeshActor
{
public:

    using AStaticMeshActor::AStaticMeshActor;

    void Receive ( FMessage InMessage ) override;
};

} // namespace LumenEngine