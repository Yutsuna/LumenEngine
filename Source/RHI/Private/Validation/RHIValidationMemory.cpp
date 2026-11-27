/**
 * @file RHIValidationMemory.cpp
 * @brief Implements validated staging allocation and buffer references.
 */

#include "RHIValidationMemory.hpp"
#include <RHI/Validation/RHIValidation.hpp>

Lumen::RHI::FRHIValidationUploadAllocator::FRHIValidationUploadAllocator ( TSharedRef<IRHIUploadAllocator> InUnderlying,
                                                                           TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
}

Lumen::RHI::FRHIValidationUploadAllocator::~FRHIValidationUploadAllocator () noexcept
{
    Buffer.Reset();
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationUploadAllocator )

Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>
Lumen::RHI::FRHIValidationUploadAllocator::WrapBuffer ( TSharedRef<IRHIBuffer> InBuffer ) noexcept /** NOLINT */
{
    if ( Buffer and UnwrapRHIObject( Buffer->Get() ) == InBuffer.Get() )
    {
        return *Buffer;
    }

    TSharedRef<IRHIBuffer> Wrapped = MakeShared<FRHIValidationBuffer>( InBuffer, State );
    Buffer.Emplace( Wrapped );
    return Wrapped;
}

Lumen::TExpected<Lumen::RHI::FRHIUploadAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationUploadAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    return Underlying->Allocate( InDesc ).Transform(
        [this] ( FRHIUploadAllocation &&InAllocation ) noexcept -> FRHIUploadAllocation
    {
        InAllocation.Buffer = WrapBuffer( InAllocation.Buffer );
        return std::move( InAllocation );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationUploadAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Underlying->Free( InAllocation );
}

Lumen::RHI::FRHIValidationReadbackAllocator::FRHIValidationReadbackAllocator ( TSharedRef<IRHIReadbackAllocator> InUnderlying,
                                                                               TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
}

Lumen::RHI::FRHIValidationReadbackAllocator::~FRHIValidationReadbackAllocator () noexcept
{
    Buffer.Reset();
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationReadbackAllocator )

Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>
Lumen::RHI::FRHIValidationReadbackAllocator::WrapBuffer ( TSharedRef<IRHIBuffer> InBuffer ) noexcept /** NOLINT */
{
    if ( Buffer and UnwrapRHIObject( Buffer->Get() ) == InBuffer.Get() )
    {
        return *Buffer;
    }
    TSharedRef<IRHIBuffer> Wrapped = MakeShared<FRHIValidationBuffer>( InBuffer, State );
    Buffer.Emplace( Wrapped );
    return Wrapped;
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationReadbackAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    return Underlying->Allocate( InDesc );
}

Lumen::TExpected<Lumen::RHI::FRHIReadbackAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationReadbackAllocator::Map ( const FRHIAllocation &InAllocation ) noexcept
{
    return Underlying->Map( InAllocation );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationReadbackAllocator::GetBuffer ( const FRHIAllocation &InAllocation ) noexcept
{
    return Underlying->GetBuffer( InAllocation )
        .Transform( [this] ( TSharedRef<IRHIBuffer> &&InBuffer ) noexcept -> TSharedRef<IRHIBuffer>
    {
        return WrapBuffer( std::move( InBuffer ) );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationReadbackAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Underlying->Free( InAllocation );
}
