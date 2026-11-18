/**
 * @file RHICommandListState.cpp
 * @brief Implements command-list lifecycle transitions.
 */

#include <RHI/Commands/RHICommandListState.hpp>
Lumen::Bool Lumen::RHI::CanRHITransitionCommandList ( const ERHICommandListState InFrom,
                                                      const ERHICommandListState InTo ) noexcept
{
    switch ( InFrom )
    {
    case ERHICommandListState::Initial:
        return InTo == ERHICommandListState::Recording;
    case ERHICommandListState::Recording:
        return InTo == ERHICommandListState::Rendering or InTo == ERHICommandListState::Executable or
               InTo == ERHICommandListState::Initial;
    case ERHICommandListState::Rendering:
        return InTo == ERHICommandListState::Recording;
    case ERHICommandListState::Executable:
        return InTo == ERHICommandListState::Pending or InTo == ERHICommandListState::Initial;
    case ERHICommandListState::Pending:
        return InTo == ERHICommandListState::Executable;
    default:
        return false;
    }
}
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::RHITransitionCommandList ( ERHICommandListState &InOutState,
                                                                                     const ERHICommandListState InTo ) noexcept
{
    if ( not CanRHITransitionCommandList( InOutState, InTo ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidState( ERHIErrorCondition::Command, "Illegal command-list lifecycle transition" ) );
    }
    InOutState = InTo;
    return {};
}
