/**
 * @file OpenGLQueryPool.cpp
 * @brief Implements GL query creation, scoped capture and result readback.
 */

#include <RHIOpenGL/Synchronization/OpenGLQueryPool.hpp>

#include <Algo/Any.hpp>
#include <Algo/ForEach.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Queries/RHIQueryType.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>

/**
 * File-local helpers
 */

namespace
{

[[nodiscard]] Lumen::RHIOpenGL::GLenum MapQueryTarget ( const Lumen::RHI::ERHIQueryType InType ) noexcept
{
    return InType == Lumen::RHI::ERHIQueryType::Timestamp ? Lumen::RHIOpenGL::GlTimestamp : Lumen::RHIOpenGL::GlSamplesPassed;
}

void DeleteQueryHandles ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                          const Lumen::TVector<Lumen::RHIOpenGL::GLuint> &InQueries ) noexcept
{
    Lumen::Algo::ForEach( InQueries,
                          [] ( const Lumen::RHIOpenGL::GLuint InHandle ) noexcept -> Lumen::Bool
    {
        return InHandle != 0U;
    }, [&InDispatch] ( const Lumen::RHIOpenGL::GLuint InHandle ) noexcept
    {
        InDispatch.DeleteQueries( 1, &InHandle );
    } );
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLQueryPool::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                             const RHI::FRHIQueryPoolDesc &InDesc,
                                             const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    if ( InDesc.Type == RHI::ERHIQueryType::PipelineStatistics )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Query,
                                                     "Pipeline statistics queries are not supported by OpenGL." ) );
    }
    LUMEN_TRY_OPTIONAL_ERROR( InContext->GetUnavailableError( "QueryPool::Create" ) );

    TSharedRef<FOpenGLQueryPool> Pool =
        MakeShared<FOpenGLQueryPool>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    TVector<GLuint> NativeQueries( static_cast<USize>( InDesc.Count ), 0U );
    GLenum NativeError               = GlNoError;
    const FGLDispatchTable &Dispatch = Pool->Context->GetDispatch();
    const RHI::FRHIExpected Created =
        Pool->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeQueries, &NativeError, &InDesc] () noexcept
    {
        Dispatch.CreateQueries( MapQueryTarget( InDesc.Type ), static_cast<GLsizei>( InDesc.Count ), NativeQueries.data() );
        NativeError = Dispatch.GetError();
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( NativeError != GlNoError )
    {
        ( void )Pool->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeQueries] () noexcept
        {
            DeleteQueryHandles( Dispatch, NativeQueries );
        } );
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Query, "OpenGL refused query pool creation." ) );
    }
    if ( Algo::AnyOf( NativeQueries, [] ( const GLuint InHandle ) noexcept -> Bool
    {
        return InHandle == 0U;
    } ) )
    {
        ( void )Pool->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeQueries] () noexcept
        {
            DeleteQueryHandles( Dispatch, NativeQueries );
        } );
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Query, "OpenGL returned an invalid query handle." ) );
    }
    Pool->Queries = NativeQueries;
    return Pool;
}

Lumen::RHIOpenGL::FOpenGLQueryPool::FOpenGLQueryPool ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                       const RHI::FRHIQueryPoolDesc &InDesc,
                                                       const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLQueryPool>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      Queries( static_cast<USize>( InDesc.Count ), 0U ), Active( static_cast<USize>( InDesc.Count ), false ),
      Pending( static_cast<USize>( InDesc.Count ), false ), Available( static_cast<USize>( InDesc.Count ), false ),
      Generations( static_cast<USize>( InDesc.Count ), 0U ), Results( static_cast<USize>( InDesc.Count ), 0ULL ),
      Availability( static_cast<USize>( InDesc.Count ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLQueryPool::~FOpenGLQueryPool () noexcept
{
    TVector<GLuint> Handles;
    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        Handles = Queries;
    }
    if ( Handles.empty() )
    {
        return;
    }
    const RHI::FRHITimelineValue Required{ Context->GetExecutor()->GetCompletedTimeline() };
    const TSharedRef<FOpenGLContext> Executor = Context->GetExecutor();
    const FGLDispatchTable *Dispatch          = &Context->GetDispatch();
    ( void )Context->GetExecutor()->DeferRelease( Required, [Executor, Dispatch, Handles] () noexcept
    {
        ( void )Executor->ExecuteOnGLThread( [Dispatch, Handles] () noexcept
        {
            DeleteQueryHandles( *Dispatch, Handles );
        } );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLQueryPool );

const Lumen::RHI::FRHIQueryPoolDesc &Lumen::RHIOpenGL::FOpenGLQueryPool::GetDesc () const noexcept
{
    return Desc;
}

void Lumen::RHIOpenGL::FOpenGLQueryPool::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( USize Index = 0U; Index < Active.size(); ++Index )
    {
        ++Generations[Index];
        Active[Index]       = false;
        Pending[Index]      = false;
        Available[Index]    = false;
        Results[Index]      = 0ULL;
        Availability[Index] = {};
    }
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLQueryPool::ReadResults ( const UInt32 InFirstQuery,
                                                  const UInt32 InQueryCount,
                                                  const TSpan<UInt64> InDestination,
                                                  const RHI::FRHITimelineValue InCompleted ) const noexcept
{
    const UInt32 ValueCount = RHI::GetRHIQueryValueCount( Desc.Type ).ValueOr( 0U );
    if ( InFirstQuery > Desc.Count or InQueryCount > Desc.Count - InFirstQuery or
         InDestination.size() < static_cast<USize>( InQueryCount ) * static_cast<USize>( ValueCount ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidQuery( "Query readback range or destination is invalid." ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( UInt32 QueryOffset = 0U; QueryOffset < InQueryCount; ++QueryOffset )
    {
        const USize Index = static_cast<USize>( InFirstQuery ) + static_cast<USize>( QueryOffset );
        if ( not Available[Index] or not RHI::IsRHICompletionReached( InCompleted, Availability[Index] ) )
        {
            return TUnexpected( RHI::FRHIError( RHI::ERHIErrorCode::InvalidState, RHI::ERHIErrorCondition::Query,
                                                "Requested query result is unavailable." ) );
        }
        for ( UInt32 ValueIndex = 0U; ValueIndex < ValueCount; ++ValueIndex )
        {
            const USize DestinationIndex =
                ( static_cast<USize>( QueryOffset ) * static_cast<USize>( ValueCount ) ) + static_cast<USize>( ValueIndex );
            InDestination[DestinationIndex] = Results[Index];
        }
    }
    return {};
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLQueryPool::GetQuery ( const UInt32 InIndex ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return InIndex < Queries.size() ? Queries[InIndex] : 0U;
}

Lumen::RHIOpenGL::GLenum Lumen::RHIOpenGL::FOpenGLQueryPool::GetNativeTarget () const noexcept
{
    return MapQueryTarget( Desc.Type );
}

Lumen::UInt32 Lumen::RHIOpenGL::FOpenGLQueryPool::GetGeneration ( const UInt32 InIndex ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return InIndex < Generations.size() ? Generations[InIndex] : 0U;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueryPool::Begin ( const UInt32 InIndex ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Queries.size() )
    {
        return TUnexpected( RHI::MakeRHIInvalidQuery( "Query index is out of range." ) );
    }
    if ( Active[InIndex] or Pending[InIndex] )
    {
        return TUnexpected( RHI::FRHIError( RHI::ERHIErrorCode::InvalidState, RHI::ERHIErrorCondition::Query,
                                            "Query is already active or awaiting its result." ) );
    }
    Active[InIndex]       = true;
    Available[InIndex]    = false;
    Results[InIndex]      = 0ULL;
    Availability[InIndex] = {};
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueryPool::End ( const UInt32 InIndex ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Queries.size() )
    {
        return TUnexpected( RHI::MakeRHIInvalidQuery( "Query index is out of range." ) );
    }
    if ( not Active[InIndex] )
    {
        return TUnexpected(
            RHI::FRHIError( RHI::ERHIErrorCode::InvalidState, RHI::ERHIErrorCondition::Query, "Query is not active." ) );
    }
    Active[InIndex]       = false;
    Pending[InIndex]      = true;
    Available[InIndex]    = false;
    Results[InIndex]      = 0ULL;
    Availability[InIndex] = {};
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueryPool::PublishResult ( const UInt32 InIndex,
                                                                             const UInt32 InGeneration,
                                                                             const UInt64 InValue,
                                                                             const RHI::FRHITimelineValue InCompletion ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Queries.size() )
    {
        return TUnexpected( RHI::MakeRHIInvalidQuery( "Query index is out of range." ) );
    }
    if ( Generations[InIndex] != InGeneration or not Pending[InIndex] )
    {
        return TUnexpected( RHI::FRHIError( RHI::ERHIErrorCode::InvalidState, RHI::ERHIErrorCondition::Query,
                                            "Query result no longer matches the recorded query." ) );
    }
    Pending[InIndex]      = false;
    Available[InIndex]    = true;
    Results[InIndex]      = InValue;
    Availability[InIndex] = InCompletion;
    return {};
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLQueryPool::GetDeviceContext () const noexcept
{
    return *Context;
}
