/**
 * @file RHITransition.cpp
 * @brief Implements resource barrier validation.
 */

#include <Commands/RHITransition.hpp>
#include <variant>
namespace
{
Lumen::Bool AreQueuesValid ( const Lumen::ERHIQueueType InSource, const Lumen::ERHIQueueType InDestination ) noexcept
{
    return InSource < Lumen::ERHIQueueType::Count and InDestination < Lumen::ERHIQueueType::Count;
}
} // namespace
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferTransition::Validate () const noexcept
{
    if ( not Buffer or not IsValidRHIResourceTransition( Before, After ) or not AreQueuesValid( SourceQueue, DestinationQueue ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer transition" ) );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITextureTransition::Validate () const noexcept
{
    if ( not Texture or not Range.IsContainedIn( Texture->GetDesc().MipCount, Texture->GetDesc().LayerCount ) or
         not IsValidRHIResourceTransition( Before, After ) or not AreQueuesValid( SourceQueue, DestinationQueue ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture transition" ) );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::ValidateRHITransition ( const FRHITransition &InTransition ) noexcept
{
    return std::visit( [] ( const auto &Transition )
    {
        return Transition.Validate();
    }, InTransition );
}
