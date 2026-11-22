/**
 * @file VulkanQueue.cpp
 * @brief Implements baseline Vulkan queue queries and idle/wait semantics.
 */

#include <RHIVulkan/Queue/VulkanQueue.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <HAL/LockGuard.hpp>

/**
 * Public
 */

Lumen::RHIVulkan::FVulkanQueue::FVulkanQueue ( RHI::FRHIObjectId InId,
                                               TSharedRef<FVulkanDeviceContext> InContext,
                                               RHI::ERHIQueueType InType,
                                               UInt32 InFamilyIndex,
                                               VkQueue InQueue ) noexcept
    : ObjectBase( MakeVulkanObjectBase<FVulkanQueue>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), QueueType( InType ), FamilyIndex( InFamilyIndex ),
      NativeQueue( InQueue )
{
    /* */
}

Lumen::RHI::ERHIQueueType Lumen::RHIVulkan::FVulkanQueue::GetType () const noexcept
{
    return QueueType;
}

Lumen::RHI::FRHITimelineValue Lumen::RHIVulkan::FVulkanQueue::GetCompletedValue () const noexcept
{
    return RHI::FRHITimelineValue{ CompletedValue.load() };
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanQueue::Submit ( const RHI::FRHISubmitDesc &InDesc ) noexcept
{
    const TOptional<RHI::FRHIError> Unavailable = Context->GetUnavailableError( "Queue::Submit" );
    if ( Unavailable.HasValue() )
    {
        return TUnexpected( *Unavailable );
    }

    LUMEN_EXPECT( Validation, InDesc.Validate() );

    return TUnexpected(
        RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue, "Vulkan queue submission is implemented in V4." ) );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHIVulkan::FVulkanQueue::WaitIdle () noexcept
{
    const TOptional<RHI::FRHIError> Unavailable = Context->GetUnavailableError( "Queue::WaitIdle" );
    if ( Unavailable.HasValue() )
    {
        return TUnexpected( *Unavailable );
    }

    HAL::TUniqueLock Lock( Context->GetQueueMutex( QueueType ) );

    const VkResult Result = vkQueueWaitIdle( NativeQueue );
    if ( Result == VK_ERROR_DEVICE_LOST )
    {
        Context->MarkLost();
    }

    LUMEN_VK_TRY( Result, RHI::ERHIErrorCondition::Queue );
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHIVulkan::FVulkanQueue::Wait ( RHI::FRHITimelineValue InValue,
                                                                                     Float64 InTimeoutSeconds ) noexcept
{
    if ( not std::isfinite( InTimeoutSeconds ) or InTimeoutSeconds < 0.0 )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Synchronization,
                                                         "Queue wait timeout must be finite and nonnegative." ) );
    }

    const TOptional<RHI::FRHIError> Unavailable = Context->GetUnavailableError( "Queue::Wait" );
    if ( Unavailable.HasValue() )
    {
        return TUnexpected( *Unavailable );
    }

    if ( not InValue.IsValid() or RHI::IsRHICompletionReached( GetCompletedValue(), InValue ) )
    {
        return {};
    }

    return TUnexpected(
        RHI::MakeRHITimeout( RHI::ERHIErrorCondition::Synchronization, "Requested timeline value has not completed." ) );
}
