/**
 * @file IRHIDescriptorAllocator.hpp
 * @brief Declares variable descriptor-range allocation.
 */

#pragma once
#include <Types/IRHIObject.hpp>
#include <Types/RHIError.hpp>
namespace Lumen

{
/**
 * @struct FRHIDescriptorAllocation
 * @brief Contiguous descriptor indices.
 */
struct FRHIDescriptorAllocation
{
    UInt32 Offset = 0U;
    UInt32 Count  = 0U;
};

/**
 * @class IRHIDescriptorAllocator
 * @brief Backend-independent descriptor range contract.
 */
class IRHIDescriptorAllocator : public IRHIObject
{
public:

    /** @brief Allocates InCount contiguous descriptors. */
    [[nodiscard]] virtual TExpected<FRHIDescriptorAllocation, FRHIError> Allocate ( UInt32 InCount ) noexcept = 0;
    /** @brief Releases a previously allocated range. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Free ( FRHIDescriptorAllocation InAllocation ) noexcept = 0;
    /** @brief Recycles ranges whose GPU use has completed. */
    virtual void Reset () noexcept = 0;
};
} // namespace Lumen
