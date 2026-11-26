/**
 * @file OpenGLStateCache.cpp
 * @brief Implements redundant-state elimination over the GL dispatch table.
 */

#include <RHIOpenGL/Commands/OpenGLStateCache.hpp>

#include <Algo/Find.hpp>

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLStateCache::FOpenGLStateCache ( const FGLDispatchTable &InDispatch ) noexcept : Dispatch( InDispatch )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLStateCache::~FOpenGLStateCache () noexcept
{
    /* */
}

void Lumen::RHIOpenGL::FOpenGLStateCache::UseProgram ( const GLuint InProgram ) noexcept
{
    if ( CurrentProgram == InProgram )
    {
        return;
    }
    if ( Dispatch.UseProgram != nullptr )
    {
        Dispatch.UseProgram( InProgram );
    }
    CurrentProgram = InProgram;
}

void Lumen::RHIOpenGL::FOpenGLStateCache::BindVertexArray ( const GLuint InVertexArray ) noexcept
{
    if ( CurrentVertexArray == InVertexArray )
    {
        return;
    }
    if ( Dispatch.BindVertexArray != nullptr )
    {
        Dispatch.BindVertexArray( InVertexArray );
    }
    CurrentVertexArray = InVertexArray;
}

void Lumen::RHIOpenGL::FOpenGLStateCache::BindBufferBase ( const GLenum InTarget,
                                                           const GLuint InIndex,
                                                           const GLuint InBuffer ) noexcept
{
    if ( const TOptional<USize> TargetIndex = GetTargetIndex( InTarget ) )
    {
        const UInt32 Slot = InIndex < 8U ? InIndex : 0U;
        if ( BoundBuffers[*TargetIndex][Slot] == InBuffer )
        {
            return;
        }
        BoundBuffers[*TargetIndex][Slot] = InBuffer;
    }
    if ( Dispatch.BindBufferBase != nullptr )
    {
        Dispatch.BindBufferBase( InTarget, InIndex, InBuffer );
    }
}

void Lumen::RHIOpenGL::FOpenGLStateCache::BindTextureUnit ( const GLuint InUnit, const GLuint InTexture ) noexcept
{
    if ( InUnit < 32U )
    {
        if ( BoundTextures[InUnit] == InTexture )
        {
            return;
        }
        BoundTextures[InUnit] = InTexture;
    }
    if ( Dispatch.BindTextureUnit != nullptr )
    {
        Dispatch.BindTextureUnit( InUnit, InTexture );
    }
}

void Lumen::RHIOpenGL::FOpenGLStateCache::BindSampler ( const GLuint InUnit, const GLuint InSampler ) noexcept
{
    if ( InUnit < 32U )
    {
        if ( BoundSamplers[InUnit] == InSampler )
        {
            return;
        }
        BoundSamplers[InUnit] = InSampler;
    }
    if ( Dispatch.BindSampler != nullptr )
    {
        Dispatch.BindSampler( InUnit, InSampler );
    }
}

void Lumen::RHIOpenGL::FOpenGLStateCache::SetViewport ( const Int32 InX,
                                                        const Int32 InY,
                                                        const GLsizei InWidth,
                                                        const GLsizei InHeight ) noexcept
{
    if ( CurrentViewport[0] == InX and CurrentViewport[1] == InY and CurrentViewport[2] == static_cast<Int32>( InWidth ) and
         CurrentViewport[3] == static_cast<Int32>( InHeight ) )
    {
        return;
    }
    if ( Dispatch.Viewport != nullptr )
    {
        Dispatch.Viewport( InX, InY, InWidth, InHeight );
    }
    CurrentViewport[0] = InX;
    CurrentViewport[1] = InY;
    CurrentViewport[2] = static_cast<Int32>( InWidth );
    CurrentViewport[3] = static_cast<Int32>( InHeight );
}

void Lumen::RHIOpenGL::FOpenGLStateCache::SetScissor (
    const Bool InbEnabled, const Int32 InX, const Int32 InY, const GLsizei InWidth, const GLsizei InHeight ) noexcept
{
    SetEnabled( GlScissorTest, InbEnabled );
    if ( CurrentScissor[0] == InX and CurrentScissor[1] == InY and CurrentScissor[2] == static_cast<Int32>( InWidth ) and
         CurrentScissor[3] == static_cast<Int32>( InHeight ) )
    {
        return;
    }
    if ( Dispatch.Scissor != nullptr )
    {
        Dispatch.Scissor( InX, InY, InWidth, InHeight );
    }
    CurrentScissor[0] = InX;
    CurrentScissor[1] = InY;
    CurrentScissor[2] = static_cast<Int32>( InWidth );
    CurrentScissor[3] = static_cast<Int32>( InHeight );
}

void Lumen::RHIOpenGL::FOpenGLStateCache::Enable ( const GLenum InCapability ) noexcept
{
    SetEnabled( InCapability, true );
}

void Lumen::RHIOpenGL::FOpenGLStateCache::Disable ( const GLenum InCapability ) noexcept
{
    SetEnabled( InCapability, false );
}

void Lumen::RHIOpenGL::FOpenGLStateCache::SetEnabled ( const GLenum InCapability, const Bool InbEnabled ) noexcept
{
    if ( FCapabilityState *Capability =
             Algo::FindIf( Capabilities, [InCapability] ( const FCapabilityState &InState ) noexcept -> Bool
    {
        return InState.Capability == InCapability;
    } ) )
    {
        if ( Capability->bEnabled == InbEnabled )
        {
            return;
        }
        Capability->bEnabled = InbEnabled;
        if ( InbEnabled )
        {
            if ( Dispatch.Enable != nullptr )
            {
                Dispatch.Enable( InCapability );
            }
        }
        else if ( Dispatch.Disable != nullptr )
        {
            Dispatch.Disable( InCapability );
        }
        return;
    }

    FCapabilityState Capability;
    Capability.Capability = InCapability;
    Capability.bEnabled   = InbEnabled;
    Capabilities.push_back( Capability );
    if ( InbEnabled )
    {
        if ( Dispatch.Enable != nullptr )
        {
            Dispatch.Enable( InCapability );
        }
    }
    else if ( Dispatch.Disable != nullptr )
    {
        Dispatch.Disable( InCapability );
    }
}

void Lumen::RHIOpenGL::FOpenGLStateCache::MemoryBarrier ( const GLbitfield InBarrierBits ) noexcept
{
    if ( InBarrierBits == 0U or Dispatch.MemoryBarrier == nullptr )
    {
        return;
    }
    Dispatch.MemoryBarrier( InBarrierBits );
}

/**
 * Private
 */

Lumen::TOptional<Lumen::USize> Lumen::RHIOpenGL::FOpenGLStateCache::GetTargetIndex ( const GLenum InTarget ) noexcept
{
    switch ( InTarget )
    {
    case GlUniformBuffer:
        return TOptional<USize>( 0U );
    case GlShaderStorageBuffer:
        return TOptional<USize>( 1U );
    case GlArrayBuffer:
        return TOptional<USize>( 2U );
    case GlElementArrayBuffer:
        return TOptional<USize>( 3U );
    case GlPixelUnpackBuffer:
        return TOptional<USize>( 4U );
    case GlPixelPackBuffer:
        return TOptional<USize>( 5U );
    default:
        return {};
    }
}
