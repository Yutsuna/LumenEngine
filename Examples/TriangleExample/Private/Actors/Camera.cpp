/**
 * @file CameraActor.cpp
 * @brief Implementation of the AExampleCameraActor class
 */

#include "Actors/Camera.hpp"

#include "Graphics/Renderer.hpp"
#include "HAL/PlatformTime.hpp"

/**
 * Ctor
 */

LumenEngine::AExampleCameraActor::AExampleCameraActor ( ActorID InId ) noexcept : ACameraActor( InId )
{
    Initialize( 60.0F, 1280.0F / 720.0F, { 0.0F, 0.5F, -2.0F } );
}

/**
 * Public
 */

void LumenEngine::AExampleCameraActor::Tick ( const Float64 InDeltaTime )
{
    const RHI::FGlobalUniformData Uniforms = { .ViewProjectionMatrix = Camera.GetViewProjectionMatrix(),
                                               .TimeSeconds          = static_cast<Float32>( HAL::FPlatformTime::Seconds() ),
                                               .DeltaTime            = static_cast<Float32>( InDeltaTime ) };

    Renderer::GRenderer->SubmitGlobalUniforms( Uniforms );
}

/**
 * Private
 */

void LumenEngine::AExampleCameraActor::Initialize ( Float32 InFov, Float32 InAspect, const Maths::FVec3f &InEye ) noexcept
{
    Camera.SetPerspective( InFov, InAspect, 0.1F, 100.0F );
    Camera.LookAt( InEye, { 0.0F, 0.0F, 0.0F } );
}
