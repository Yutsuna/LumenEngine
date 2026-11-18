/**
 * @file NullModule.hpp
 * @brief Declares the Null RHI module.
 */
#pragma once

#include <Device/NullDeviceDesc.hpp>
#include <RHI/Module/IRHIModule.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullModule
         * @brief Deterministic adapter discovery and device creation.
         */
        class FNullModule final : public IRHIModule
        {
        public:

            /** @brief Creates a module with one configurable Null adapter. */
            explicit FNullModule ( FNullDeviceDesc InDesc = {} ) noexcept;

            [[nodiscard]] FRHIApiVersion GetApiVersion () const noexcept override;
            [[nodiscard]] TExpected<TVector<FRHIAdapterInfo>, FRHIError> EnumerateAdapters () const noexcept override;
            [[nodiscard]] TExpected<TSharedRef<IRHIDevice>, FRHIError>
            CreateDevice ( const FRHIDeviceDesc &InDesc ) noexcept override;

        private:

            FNullDeviceDesc Desc;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
