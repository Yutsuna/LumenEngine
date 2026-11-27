/**
 * @file RHIDefragmenter.hpp
 * @brief Declares the backend-independent defragmenter.
 */

#pragma once

#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIDefragmenter
     * @brief Conservative planner that emits only validated moves.
     */
    class RHI_EXPORT FRHIDefragmenter final : public IRHIDefragmenter
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates a defragmenter identity. */
        explicit FRHIDefragmenter ( FRHIObjectId InId ) noexcept;

        TExpected<FRHIDefragPlan, FRHIError> BuildPlan ( TSpan<const FRHIAllocation> InAllocations ) noexcept override;

        FRHIExpected Execute ( const FRHIDefragPlan &InPlan ) noexcept override;

    private:

        FRHIObjectBase Identity;
    };
} // namespace RHI

} // namespace Lumen
