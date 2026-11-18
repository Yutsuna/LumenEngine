/**
 * @file RHIHeapDesc.hpp
 * @brief Defines native heap creation parameters.
 */
#pragma once

#include <Memory/RHIMemoryClass.hpp>
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIFlags.hpp>

namespace Lumen
{

/**
 * @struct FRHIHeapDesc
 * @brief Size, memory class, and object flags of a heap.
 */
struct RHI_EXPORT FRHIHeapDesc
{
    UInt64 Size                 = 0ULL;
    ERHIMemoryClass MemoryClass = ERHIMemoryClass::DeviceLocal;
    ERHIObjectFlags Flags       = ERHIObjectFlags::None;
    /** @return Success for a nonzero heap size. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
