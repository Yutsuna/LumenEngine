/**
 * @file RHIDescriptorAllocatorBase.hpp
 * @brief Declares the backend-neutral descriptor allocator implementation.
 */
#pragma once
#include <Bindings/IRHIDescriptorAllocator.hpp>
#include <Bindings/IRHIDescriptorPoolBackend.hpp>
#include <Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Memory/IndiceAllocator.hpp>
#include <Memory/UniquePtr.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{

/**
 * @class FRHIDescriptorAllocatorBase
 * @brief Common free-list and native pool growth for backends.
 */
class RHI_EXPORT FRHIDescriptorAllocatorBase : public IRHIDescriptorAllocator
{
public:

    /** @brief Uses a validated policy and a backend pool provider. */
    FRHIDescriptorAllocatorBase ( const FRHIDescriptorAllocatorDesc &InDesc, IRHIDescriptorPoolBackend &InBackend ) noexcept;
    ~FRHIDescriptorAllocatorBase () noexcept override;
    TExpected<FRHIDescriptorAllocation, FRHIError> Allocate ( UInt32 InCount ) noexcept override;
    TExpected<void, FRHIError> Free ( FRHIDescriptorAllocation InAllocation ) noexcept override;
    void Reset () noexcept override;

protected:

    /** @brief Releases all native pools after external GPU completion. */
    void ReleasePools () noexcept;

private:

    struct FPool
    {
        UInt32 BaseOffset  = 0U;
        UInt64 BackendPool = 0ULL;
        TUniquePtr<FIndiceAllocator> Indices;
    };

private:

    FRHIDescriptorAllocatorDesc Desc;
    IRHIDescriptorPoolBackend *Backend;
    TVector<FPool> Pools;
    TVector<FRHIDescriptorAllocation> Active;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
