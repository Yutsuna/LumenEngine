/**
 * @file RenderBackendSelection.hpp
 * @brief Private backend discovery, device and swapchain creation.
 */

#pragma once

#include <Execution/Renderer.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <String/String.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @struct FSelectedRenderBackend
 * @brief Backend-owned objects kept entirely inside Renderer/Private.
 */
struct FSelectedRenderBackend final
{
    TSharedPtr<RHI::IRHIModule> Module;
    TOptional<TSharedRef<RHI::IRHIDevice>> Device;
    TSharedPtr<RHI::IRHISwapchain> Swapchain;
    Bool bSoftwareAdapter = false;
    FString AdapterName;
};

/**
 * @brief Selects a backend from the opaque preference string and creates its device.
 * @param InDesc Neutral renderer description carrying the opaque surface handles.
 * @param InExtent Concrete RHI extent derived from the neutral extent.
 * @param InInjectedDevice Optional device supplied by the internal test hook.
 */
[[nodiscard]] TExpected<FSelectedRenderBackend, ERendererError>
SelectRenderBackend ( const FRendererDesc &InDesc,
                      const RHI::FRHIExtent3D &InExtent,
                      TOptional<TSharedRef<RHI::IRHIDevice>> InInjectedDevice = {} ) noexcept;

} // namespace Lumen
