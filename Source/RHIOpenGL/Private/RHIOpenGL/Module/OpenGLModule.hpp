/**
 * @file OpenGLModule.hpp
 * @brief Declares the OpenGL RHI module implementation.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <RHI/Module/IRHIModule.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLModule
     * @brief Discovers headless OpenGL 4.6 adapters and creates devices.
     */
    class FOpenGLModule final : public RHI::IRHIModule, public FNonCopyable, public FNonMovable
    {
    public:

        FOpenGLModule () noexcept           = default;
        ~FOpenGLModule () noexcept override = default;

        /** @brief Creates an OpenGL RHI module instance. */
        [[nodiscard]] static TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Create () noexcept;

        [[nodiscard]] RHI::FRHIApiVersion GetApiVersion () const noexcept override;

        [[nodiscard]] TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> EnumerateAdapters () const noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError>
        CreateDevice ( const RHI::FRHIDeviceDesc &InDesc ) noexcept override;
    };

} // namespace RHIOpenGL

} // namespace Lumen
