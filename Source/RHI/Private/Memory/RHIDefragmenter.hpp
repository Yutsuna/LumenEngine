/**
 * @file RHIDefragmenter.hpp
 * @brief Declares the backend-independent defragmenter.
 */

#pragma once

#include "../Types/RHIObjectForward.hpp"
#include <RHI/Memory/IRHIDefragmenter.hpp>

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
        TExpected<void, FRHIError> Execute ( const FRHIDefragPlan &InPlan ) noexcept override;

    private:

        FRHIObjectBase Identity;
    };
} // namespace RHI

} // namespace Lumen
