/**
 * @file RHIAllocationDesc.hpp
 * @brief Defines allocator input requirements.
 */

#pragma once

#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @struct FRHIAllocationDesc
     * @brief Requested size, alignment, and memory class.
     */
    struct RHI_EXPORT FRHIAllocationDesc
    {
        UInt64 Size                 = 0ULL;
        UInt64 Alignment            = 1ULL;
        ERHIMemoryClass MemoryClass = ERHIMemoryClass::DeviceLocal;
        /** @return Success for nonzero size and power-of-two alignment. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
