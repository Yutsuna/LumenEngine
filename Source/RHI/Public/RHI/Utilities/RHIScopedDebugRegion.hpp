/**
 * @file RHIScopedDebugRegion.hpp
 * @brief Declares a RAII command-list debug region.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <String/StringView.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHICommandList;

    /**
     * @class FRHIScopedDebugRegion
     * @brief Closes a debug region at scope exit.
     */
    class FRHIScopedDebugRegion final : public FNonCopyable
    {
    public:

        /** @brief Begins a named region on InCommandList. */
        FRHIScopedDebugRegion ( IRHICommandList &InCommandList, FStringView InName ) noexcept;
        /** @brief Ends the region without throwing. */
        ~FRHIScopedDebugRegion () noexcept;

    private:

        IRHICommandList *CommandList;
    };

} // namespace RHI

} // namespace Lumen

#include "Inline/RHIScopedDebugRegion.inl"
