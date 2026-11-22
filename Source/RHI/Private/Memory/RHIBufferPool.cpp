/**
 * @file RHIBufferPool.cpp
 * @brief Implements device-backed buffer acquisition and safe retention.
 */

#include <Memory/RHIBufferPool.hpp>

#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHITimeline.hpp>
#include <Templates/Hash.hpp>

namespace
{

/** @return True when two descriptors describe interchangeable buffer resources. */
LUMEN_FORCE_INLINE Lumen::Bool AreBufferDescsCompatible ( const Lumen::RHI::FRHIBufferDesc &InLeft,
                                                          const Lumen::RHI::FRHIBufferDesc &InRight ) noexcept
{
    const Lumen::Bool bIsSameSize         = InLeft.Size == InRight.Size;
    const Lumen::Bool bIsSameUsage        = InLeft.Usage == InRight.Usage;
    const Lumen::Bool bIsSameMemoryClass  = InLeft.MemoryClass == InRight.MemoryClass;
    const Lumen::Bool bIsSameInitialState = InLeft.InitialState == InRight.InitialState;

    return bIsSameSize and bIsSameUsage and bIsSameMemoryClass and bIsSameInitialState;
}

} // namespace

/**
 * Public
 */

Lumen::RHI::FRHIBufferPool::FRHIBufferPool ( IRHIDevice &InDevice, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIBufferPool>() ), Device( &InDevice )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( FRHIBufferPool )
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIBufferPool::Acquire ( const FRHIBufferDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( _, InDesc.Validate() );

    for ( USize Index = 0U; Index < Released.size(); ++Index )
    {
        const FEntry &Entry = Released[Index];
        if ( IsRHICompletionReached( CurrentCompleted, Entry.Completion ) and
             AreBufferDescsCompatible( Entry.Buffer->GetDesc(), InDesc ) )
        {
            TSharedRef<IRHIBuffer> Result = Entry.Buffer;
            Released.erase( Released.begin() + static_cast<ISize>( Index ) );
            return Result;
        }
    }
    return Device->CreateBuffer( InDesc );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIBufferPool::Release ( TSharedRef<IRHIBuffer> InBuffer, const FRHITimelineValue InCompletion ) noexcept
{
    Released.push_back( FEntry{ .Buffer = std::move( InBuffer ), .Completion = InCompletion } );
    return {};
}

void Lumen::RHI::FRHIBufferPool::AdvanceCompleted ( const FRHITimelineValue InCompleted ) noexcept
{
    if ( InCompleted > CurrentCompleted )
    {
        CurrentCompleted = InCompleted;
    }
}

Lumen::USize Lumen::RHI::FRHIBufferPool::GetPendingCount () const noexcept
{
    return Released.size();
}
