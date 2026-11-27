/**
 * @file RHITransition.cpp
 * @brief Implements resource barrier validation.
 */

#include <RHI/Commands/RHITransition.hpp>
#include <variant>
namespace
{
Lumen::Bool AreQueuesValid ( const Lumen::RHI::ERHIQueueType InSource, const Lumen::RHI::ERHIQueueType InDestination ) noexcept
{
    return InSource < Lumen::RHI::ERHIQueueType::Count and InDestination < Lumen::RHI::ERHIQueueType::Count;
}
} // namespace
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIBufferTransition::Validate () const noexcept
{
    if ( not Buffer or not IsValidRHIResourceTransition( Before, After ) or not AreQueuesValid( SourceQueue, DestinationQueue ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer transition" ) );
    }
    return {};
}
Lumen::RHI::FRHIExpected Lumen::RHI::FRHITextureTransition::Validate () const noexcept
{
    if ( not Texture or not Range.IsContainedIn( Texture->GetDesc().MipCount, Texture->GetDesc().LayerCount ) or
         not IsValidRHIResourceTransition( Before, After ) or not AreQueuesValid( SourceQueue, DestinationQueue ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture transition" ) );
    }
    return {};
}
Lumen::RHI::FRHIExpected Lumen::RHI::ValidateRHITransition ( const FRHITransition &InTransition ) noexcept
{
    return std::visit( [] ( const auto &Transition )
    {
        return Transition.Validate();
    }, InTransition );
}
