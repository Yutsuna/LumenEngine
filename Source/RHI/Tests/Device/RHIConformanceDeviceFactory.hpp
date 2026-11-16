/**
 * @file RHIConformanceDeviceFactory.hpp
 * @brief Declares the injectable factory shared by backend conformance suites.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <Module/IRHIModule.hpp>

namespace Lumen
{

namespace Tests
{

    /**
     * @class IRHIConformanceDeviceFactory
     * @brief Supplies one backend to portable scenarios.
     */
    class IRHIConformanceDeviceFactory
    {
    public:

        virtual ~IRHIConformanceDeviceFactory () noexcept = default;

        /** @return The backend module used for adapter discovery. */
        [[nodiscard]] virtual TSharedRef<IRHIModule> GetModule () const noexcept = 0;
        /** @brief Creates a device, optionally enclosed by validation. */
        [[nodiscard]] virtual TExpected<TSharedRef<IRHIDevice>, FRHIError>
        CreateDevice ( Bool bEnableValidation = false ) noexcept = 0;
    };

    /** @return The factory linked into the current backend test binary. */
    [[nodiscard]] IRHIConformanceDeviceFactory &GetRHIConformanceDeviceFactory () noexcept;

} // namespace Tests

} // namespace Lumen
