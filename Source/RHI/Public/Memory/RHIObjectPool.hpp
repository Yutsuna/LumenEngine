/**
 * @file RHIObjectPool.hpp
 * @brief Declares a fixed-capacity CPU object pool.
 */
#pragma once

#include <Core/NonCopyable.hpp>
#include <Memory/IndiceAllocator.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @class TRHIObjectPool
 * @brief Reuses aligned slots without per-object heap allocation.
 */
template <typename ObjectType> class TRHIObjectPool final : public FNonCopyable
{
public:

    /** @brief Creates InCapacity aligned object slots. */
    explicit TRHIObjectPool ( UInt32 InCapacity ) noexcept;
    /** @brief Constructs an object in a free slot, or returns an empty optional when the pool is full. */
    template <typename... ArgumentTypes> TOptional<ObjectType *> Create ( ArgumentTypes &&...InArguments ) noexcept;
    /** @brief Destroys an object created by this pool and releases its slot. */
    void Destroy ( ObjectType *InObject ) noexcept;

private:

    struct alignas( ObjectType ) FSlot
    {
        Byte Storage[sizeof( ObjectType )];
    };
    TUniquePtr<FSlot[]> Objects;
    FIndiceHeapAllocator Slots;
};
} // namespace Lumen
#include "Inline/RHIObjectPool.inl"
