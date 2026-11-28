/**
 * @file StartWithTestDevice.hpp
 * @brief Internal test hook: starts a renderer with an injected RHI device.
 * @details Tests compile against Renderer/Private and therefore see RHI directly. No device type
 *          appears in Renderer/Public and no factory is exposed to clients.
 */

#pragma once

#include <Execution/Renderer.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/** @brief Starts InRenderer using InDevice instead of a production backend device. */
[[nodiscard]] TExpected<void, ERendererError>
StartWithTestDevice ( FRenderer &InRenderer,
                      TSharedRef<RHI::IRHIDevice> InDevice,
                      const FRendererDesc &InDesc,
                      TSharedPtr<RHI::IRHISwapchain> InSwapchain = nullptr ) noexcept;

} // namespace Lumen
