/**
 * @file RHIValidationMemory.hpp
 * @brief Declares validation decorators for staging allocators.
 */
#pragma once

#include "RHIValidationResources.hpp"
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @class FRHIValidationUploadAllocator
     * @brief Exposes validated buffer references for upload allocations.
     */
    class FRHIValidationUploadAllocator final : public IRHIUploadAllocator, public IRHIValidationObject
    {
    public:

        FRHIValidationUploadAllocator ( TSharedRef<IRHIUploadAllocator> InUnderlying,
                                        TSharedPtr<FRHIValidationState> InState ) noexcept;
        ~FRHIValidationUploadAllocator () noexcept override;
        [[nodiscard]] FRHIObjectId GetId () const noexcept override;
        [[nodiscard]] FString GetDebugName () const noexcept override;
        void SetDebugName ( FStringView InName ) noexcept override;
        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
        [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
        [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
        TExpected<FRHIUploadAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
        FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept override;

    private:

        [[nodiscard]] TSharedRef<IRHIBuffer> WrapBuffer ( TSharedRef<IRHIBuffer> InBuffer ) noexcept;

    private:

        TSharedRef<IRHIUploadAllocator> Underlying;
        TSharedPtr<FRHIValidationState> State;
        TOptional<TSharedRef<IRHIBuffer>> Buffer;
    };

    /**
     * @class FRHIValidationReadbackAllocator
     * @brief Exposes a validated copy destination for readback allocations.
     */
    class FRHIValidationReadbackAllocator final : public IRHIReadbackAllocator, public IRHIValidationObject
    {
    public:

        FRHIValidationReadbackAllocator ( TSharedRef<IRHIReadbackAllocator> InUnderlying,
                                          TSharedPtr<FRHIValidationState> InState ) noexcept;
        ~FRHIValidationReadbackAllocator () noexcept override;
        [[nodiscard]] FRHIObjectId GetId () const noexcept override;
        [[nodiscard]] FString GetDebugName () const noexcept override;
        void SetDebugName ( FStringView InName ) noexcept override;
        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
        [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
        [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
        TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
        TExpected<FRHIReadbackAllocation, FRHIError> Map ( const FRHIAllocation &InAllocation ) noexcept override;
        TExpected<TSharedRef<IRHIBuffer>, FRHIError> GetBuffer ( const FRHIAllocation &InAllocation ) noexcept override;
        FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept override;

    private:

        [[nodiscard]] TSharedRef<IRHIBuffer> WrapBuffer ( TSharedRef<IRHIBuffer> InBuffer ) noexcept;

    private:

        TSharedRef<IRHIReadbackAllocator> Underlying;
        TSharedPtr<FRHIValidationState> State;
        TOptional<TSharedRef<IRHIBuffer>> Buffer;
    };
} // namespace RHI

} // namespace Lumen
