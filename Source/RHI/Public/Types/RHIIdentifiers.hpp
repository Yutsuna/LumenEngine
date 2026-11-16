/**
 * @file RHIIdentifiers.hpp
 * @brief Defines strongly typed RHI object identifiers.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <compare>

namespace Lumen
{

/**
 * @class TRHIId
 * @brief Strong identifier parameterized by an object-category tag.
 */
template <typename TagType> struct TRHIId
{
    UInt64 Value = 0ULL;

    /** @return True when this is not the invalid zero identifier. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;
    /** @return Raw value for hashing and serialization only. */
    [[nodiscard]] constexpr UInt64 GetValue () const noexcept;
    friend constexpr auto operator<=>( const TRHIId &, const TRHIId & ) = default;
};

struct FRHIObjectIdTag;
struct FRHIDeviceIdTag;
struct FRHIBufferIdTag;
struct FRHITextureIdTag;
struct FRHITextureViewIdTag;
struct FRHISamplerIdTag;
struct FRHIHeapIdTag;
struct FRHIBindingIdTag;
struct FRHIPipelineIdTag;
struct FRHIShaderIdTag;
struct FRHICommandIdTag;
struct FRHISemaphoreIdTag;
struct FRHISwapchainIdTag;
struct FRHIQueryIdTag;
struct FRHIAllocationIdTag;

using FRHIObjectId      = TRHIId<FRHIObjectIdTag>;
using FRHIDeviceId      = TRHIId<FRHIDeviceIdTag>;
using FRHIBufferId      = TRHIId<FRHIBufferIdTag>;
using FRHITextureId     = TRHIId<FRHITextureIdTag>;
using FRHITextureViewId = TRHIId<FRHITextureViewIdTag>;
using FRHISamplerId     = TRHIId<FRHISamplerIdTag>;
using FRHIHeapId        = TRHIId<FRHIHeapIdTag>;
using FRHIBindingId     = TRHIId<FRHIBindingIdTag>;
using FRHIPipelineId    = TRHIId<FRHIPipelineIdTag>;
using FRHIShaderId      = TRHIId<FRHIShaderIdTag>;
using FRHICommandId     = TRHIId<FRHICommandIdTag>;
using FRHISemaphoreId   = TRHIId<FRHISemaphoreIdTag>;
using FRHISwapchainId   = TRHIId<FRHISwapchainIdTag>;
using FRHIQueryId       = TRHIId<FRHIQueryIdTag>;
using FRHIAllocationId  = TRHIId<FRHIAllocationIdTag>;

/**
 * @class FRHIIdentifierGenerator
 * @brief Thread-safe monotonic generator for one device.
 */
class RHI_EXPORT FRHIIdentifierGenerator
{
public:

    /** @brief Starts identifiers at one; zero remains invalid. */
    FRHIIdentifierGenerator () noexcept;

    /** @return The next identifier, or an invalid identifier after UInt64 exhaustion. */
    template <typename TagType> [[nodiscard]] TRHIId<TagType> Next () noexcept;

private:

    TAtomic<UInt64> NextValue;
};

} // namespace Lumen

#include "Inline/RHIIdentifiers.inl"
