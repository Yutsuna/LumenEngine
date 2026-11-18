/**
 * @file IRHIShader.hpp
 * @brief Declares an immutable backend shader module.
 */
#pragma once
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>
namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIShader
     * @brief Shader module with portable reflection.
     */
    class IRHIShader : public IRHIObject
    {
    public:

        /** @return Creation description valid for the shader lifetime. */
        [[nodiscard]] virtual const FRHIShaderDesc &GetDesc () const noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
