/**
 * @file IRHIModule.hpp
 * @brief Declares the entry point implemented by every RHI backend module.
 */
#pragma once

#include <RHIExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Memory/SharedPtr.hpp>
#include <Module/RHIAdapterInfo.hpp>
#include <Module/RHIDeviceDesc.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>
#include <compare>

namespace Lumen
{

class IRHIDevice;

/**
 * @struct FRHIApiVersion
 * @brief Version of the public RHI contract.
 */
struct FRHIApiVersion
{
    UInt32 Major = 1U;
    UInt32 Minor = 0U;
    UInt32 Patch = 0U;

    friend constexpr auto operator<=>( const FRHIApiVersion &, const FRHIApiVersion & ) = default;
};

/**
 * @class IRHIModule
 * @brief Backend module responsible for discovery and device creation.
 */
class RHI_EXPORT IRHIModule
{
public:

    virtual ~IRHIModule () noexcept = default;

    /** @return API version implemented by this module. */
    [[nodiscard]] virtual FRHIApiVersion GetApiVersion () const noexcept = 0;

    /** @return All currently visible adapters, or a discovery failure. */
    [[nodiscard]] virtual TExpected<TVector<FRHIAdapterInfo>, FRHIError> EnumerateAdapters () const noexcept = 0;

    /** @brief Creates a device after validating the requested adapter and capabilities. */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIDevice>, FRHIError> CreateDevice ( const FRHIDeviceDesc &InDesc ) noexcept = 0;
};

} // namespace Lumen
