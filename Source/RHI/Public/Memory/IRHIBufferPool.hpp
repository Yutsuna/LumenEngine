/**
 * @file IRHIBufferPool.hpp
 * @brief Declares buffer recycling by compatible descriptor.
 */
#pragma once
#include <Memory/SharedPtr.hpp>
#include <Types/IRHIObject.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHITimeline.hpp>
namespace Lumen
{
class IRHIBuffer;
struct FRHIBufferDesc;

/**
 * @class IRHIBufferPool
 * @brief Device-created pool of reusable buffers.
 */
class IRHIBufferPool : public IRHIObject
{
public:

    /** @brief Acquires a buffer matching InDesc. */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBuffer>, FRHIError> Acquire ( const FRHIBufferDesc &InDesc ) noexcept = 0;
    /** @brief Returns a buffer for reuse after its completion value. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Release ( TSharedRef<IRHIBuffer> InBuffer,
                                                               FRHITimelineValue InCompletion ) noexcept = 0;
};
} // namespace Lumen
