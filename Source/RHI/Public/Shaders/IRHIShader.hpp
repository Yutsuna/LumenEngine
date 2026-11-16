/**
 * @file IRHIShader.hpp
 * @brief Declares an immutable backend shader module.
 */
#pragma once
#include <Shaders/RHIShaderDesc.hpp>
#include <Types/IRHIObject.hpp>
namespace Lumen
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
} // namespace Lumen
