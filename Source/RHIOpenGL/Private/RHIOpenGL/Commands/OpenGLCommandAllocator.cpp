/**
 * @file OpenGLCommandAllocator.cpp
 * @brief Implements OpenGL command allocator tracking and recycling.
 */

#include <RHIOpenGL/Commands/OpenGLCommandAllocator.hpp>

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLCommandAllocator::FOpenGLCommandAllocator ( const RHI::ERHIQueueType InQueueType,
                                                                     const RHI::FRHIObjectId InId ) noexcept
    : RHI::FRHICommandAllocatorBase( InQueueType ), Identity( MakeOpenGLObjectBase<FOpenGLCommandAllocator>( InId ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLCommandAllocator::~FOpenGLCommandAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLCommandAllocator );

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandAllocator::Reset ( const RHI::FRHITimelineValue InCompleted ) noexcept
{
    if ( const RHI::FRHIExpected ResetLists = RHI::FRHICommandAllocatorBase::Reset( InCompleted ); not ResetLists )
    {
        return TUnexpected( ResetLists.Error() );
    }
    for ( FOpenGLCommandList *List : Lists )
    {
        ( void )List->MarkCompleted( InCompleted );
    }
    return {};
}

void Lumen::RHIOpenGL::FOpenGLCommandAllocator::TrackList ( FOpenGLCommandList &InList ) noexcept
{
    RegisterList( InList );
    Lists.push_back( &InList );
}

void Lumen::RHIOpenGL::FOpenGLCommandAllocator::UntrackList ( FOpenGLCommandList &InList ) noexcept
{
    UnregisterList( InList );
    for ( USize Index = Lists.size(); Index > 0U; --Index )
    {
        if ( Lists[Index - 1U] == &InList )
        {
            Lists.erase( Lists.begin() + static_cast<decltype( Lists )::difference_type>( Index - 1U ) );
        }
    }
}
