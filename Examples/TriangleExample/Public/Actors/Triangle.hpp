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

    void Receive ( const FMessage &InMessage ) override;

private:

    void RotateTriangle ( const Float64 InDeltaTime ) noexcept;
    void DrawTriangle () noexcept;
};

} // namespace LumenEngine
