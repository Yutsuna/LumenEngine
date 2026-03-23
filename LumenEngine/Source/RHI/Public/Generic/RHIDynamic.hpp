/**
 * @file RHIDynamic.hpp
 * @brief Interface for the dynamic RHI, which abstracts the underlying graphics API (e.g., Vulkan, DirectX, OpenGL).
 */

#pragma once

#include "RHIViewport.hpp"

#include "Container/SharedPtr.hpp"
#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace RHI
{

    class LUMEN_ENGINE_API IFRHIDynamic
    {
    public:

        virtual ~IFRHIDynamic () = default;

    public:

        /** Initializes the RHI (e.g., creates Instance, Physical Device, Logical Device) */
        virtual void Init () = 0;

        /** Cleans up the RHI resources */
        virtual void Shutdown () = 0;

        /** Creates a viewport linked to an OS-specific window handle */
        virtual TSharedRef<IFRHIViewport> RHICreateViewport ( void *InWindowHandle, const Maths::FVec2u &InSize ) = 0;
    };

    extern LUMEN_ENGINE_API IFRHIDynamic *GDynamicRHI;

} // namespace RHI

} // namespace LumenEngine
