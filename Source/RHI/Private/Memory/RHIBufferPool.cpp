/**
 * @file RHIBufferPool.cpp
 * @brief Implements device-backed buffer acquisition and safe retention.
 */

#include "RHIBufferPool.hpp"
#include <Resources/IRHIBuffer.hpp>
#include <Templates/Hash.hpp>
#include <Types/RHITimeline.hpp>

namespace
{

/** @return True when two descriptors describe interchangeable buffer resources. */
LUMEN_FORCE_INLINE Lumen::Bool AreBufferDescsCompatible ( const Lumen::FRHIBufferDesc &InLeft,
                                                          const Lumen::FRHIBufferDesc &InRight ) noexcept
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

Lumen::FRHIBufferPool::FRHIBufferPool ( IRHIDevice &InDevice, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIBufferPool>() ), Device( &InDevice )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( FRHIBufferPool )
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBuffer>, Lumen::FRHIError>
Lumen::FRHIBufferPool::Acquire ( const FRHIBufferDesc &InDesc ) noexcept
{
    if ( const TExpected<void, FRHIError> Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
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

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferPool::Release ( TSharedRef<IRHIBuffer> InBuffer,
                                                                          const FRHITimelineValue InCompletion ) noexcept
{
    Released.push_back( FEntry{ .Buffer = std::move( InBuffer ), .Completion = InCompletion } );
    return {};
}

void Lumen::FRHIBufferPool::AdvanceCompleted ( const FRHITimelineValue InCompleted ) noexcept
{
    if ( InCompleted > CurrentCompleted )
    {
        CurrentCompleted = InCompleted;
    }
}

Lumen::USize Lumen::FRHIBufferPool::GetPendingCount () const noexcept
{
    return Released.size();
}
