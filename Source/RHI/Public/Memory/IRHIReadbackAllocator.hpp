/**
 * @file IRHIReadbackAllocator.hpp
 * @brief Declares GPU readback allocation.
 */
#pragma once
#include <Memory/IRHIAllocator.hpp>
#include <Memory/Span.hpp>
#include <Types/IRHIObject.hpp>
namespace Lumen

{
/**
 * @struct FRHIReadbackAllocation
 * @brief Readback metadata and readable bytes after completion.
 */
struct FRHIReadbackAllocation
{
    FRHIAllocation Allocation;
    TSpan<const Byte> MappedData;
};

/**
 * class IRHIReadbackAllocator @bri
 * f Readback ring whose ranges are recycled after completion.
 */
class IRHIReadbackAllocator : public IRHIObject
{
public:

    /** @brief Allocates a readback destination. */
    [[nodiscard]] virtual TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept = 0;
    /** @brief Maps a completed readback range. */
    [[nodiscard]] virtual TExpected<FRHIReadbackAllocation, FRHIError> Map ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Releases a readback range. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept = 0;
};
} // namespace Lumen
