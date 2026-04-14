/**
 * @file CameraActor.hpp
 * @brief Declaration of the AExampleCameraActor class
 */

#pragma once

#include "Actors/CameraActor.hpp"

namespace LumenEngine
{

/**
 * @class AExampleCameraActor
 * @brief Camera actor for the TriangleExample
 */
class AExampleCameraActor final : public Engine::ACameraActor
{
public:

    using ACameraActor::ACameraActor;

    AExampleCameraActor ( ActorID InId ) noexcept;

protected:

    void Tick ( const Float64 InDeltaTime ) override;

private:

    void Initialize ( Float32 InFov, Float32 InAspect, const Maths::FVec3f &InEye ) noexcept;
};

} // namespace LumenEngine