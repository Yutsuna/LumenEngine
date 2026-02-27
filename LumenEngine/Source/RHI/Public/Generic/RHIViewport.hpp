/**
 * @file RHIViewport.hpp
 * @brief Abstraction for a rendering viewport (Swapchain in Vulkan).
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

namespace RHI
{

    class LUMEN_ENGINE_API IFRHIViewport
    {
    public:

        virtual ~IFRHIViewport () = default;

    public:

        /** Retrieves the width of the viewport. */
        virtual UInt32 GetWidth () const = 0;

        /** Retrieves the height of the viewport. */
        virtual UInt32 GetHeight () const = 0;
    };

} // namespace RHI

} // namespace LumenEngine
