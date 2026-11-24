/**
 * @file RHIHeapDesc.hpp
 * @brief Defines native heap creation parameters.
 */
#pragma once

#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFlags.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
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
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
