/**
 * @file RHIOpenGLModule.cpp
 * @brief Implementation of the RHIOpenGL module entry point.
 */

#include <RHIOpenGL/RHIOpenGLModule.hpp>

#include "Module/OpenGLModule.hpp"

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIModule>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::CreateRHIOpenGLModule () noexcept
{
    return FOpenGLModule::Create();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIModule>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::CreateRHIOpenGLModule ( const FOpenGLNativeWindowDesc &InNativeWindow ) noexcept
{
    return FOpenGLModule::Create( &InNativeWindow );
}
