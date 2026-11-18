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

namespace RHI
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

        /**
         * @brief Starts identifiers at a caller-supplied value.
         * @details Narrow seam for exercising the exhaustion boundary without iterating the whole
         *          UInt64 range. Values 1..max are issued as valid identifiers; zero starts the
         *          generator already in the exhausted state.
         * @param InFirstValue First identifier this generator will issue.
         */
        explicit FRHIIdentifierGenerator ( const UInt64 InFirstValue ) noexcept;

        /**
         * @brief Returns the next identifier for the requested category.
         * @details Issues every value from the starting value up to and including UInt64::max()
         *          exactly once, in increasing order. UInt64::max() is issued as the final valid
         *          identifier; the generator then permanently latches into the exhausted state and
         *          every subsequent call returns an invalid (zero) identifier. Zero is never issued
         *          as a valid identifier, so no value is ever reissued. Thread-safe via an atomic
         *          compare-and-swap loop.
         * @return The next valid identifier, or an invalid identifier once exhausted.
         */
        template <typename TagType> [[nodiscard]] TRHIId<TagType> Next () noexcept;

    private:

        /** NOTE: Zero is reserved as the latched exhausted sentinel; valid identifiers are 1..max. */
        TAtomic<UInt64> NextValue;
    };

} // namespace RHI

} // namespace Lumen

#include "Inline/RHIIdentifiers.inl"
