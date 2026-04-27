/**
 * @file ShaderCompilerRequest.hpp
 * @brief Definition of FShaderCompileRequest and its builder.
 */

#pragma once

#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

/**
 * @struct FShaderCompileRequest
 */
struct FShaderCompileRequest
{
    /** Path to the shader source file to compile (e.g., "Shaders/PBR.frag") */
    FString SourcePath;

    /** The shader stage to compile for (e.g., vertex, fragment, compute) */
    EShaderStage::Type Stage = EShaderStage::Vertex;

    /** The GLSL profile to compile for (e.g., Core, ES, Vulkan) */
    EGlslProfile Profile = EGlslProfile::Vulkan;

    /** The optimization level to use during compilation (e.g., None, Size, Performance) */
    EShaderOptimizationLevel Optimization = EShaderOptimizationLevel::Performance;

    /** List of preprocessor macros to define during compilation (e.g., { "USE_IBL", "1" }) */
    TVector<FShaderMacro> Macros;

    /** The entry point function name in the shader source (default is "main") */
    FString EntryPoint = "main";

    /** The GLSL version to target (e.g., 450, 460). Ignored if Profile is not Vulkan. */
    Int32 GlslVersion = 460;

    /** Additional include directories to search for #include directives in the shader source. */
    TVector<FString> IncludeDirectories;

    /** When true, the compiler will emit debug information in the SPIR-V binary (e.g., OpLine instructions). */
    Bool bEmitDebugInfo = false;
};

/**
 * @class FShaderCompileRequestBuilder
 * @brief Fluent builder to create FShaderCompileRequest with minimal boilerplate.
 */
class LUMEN_ENGINE_API FShaderCompileRequestBuilder final
{
public:

    FShaderCompileRequestBuilder () = default;

public:

    /** @brief Set the source path */
    FShaderCompileRequestBuilder &Path ( FString InPath ) noexcept;

    /** @brief Set the entry point */
    FShaderCompileRequestBuilder &EntryPoint ( FString InName ) noexcept;

    /** @brief Set the shader stage */
    FShaderCompileRequestBuilder &Stage ( EShaderStage::Type InStage ) noexcept;

public:

    /** @brief Set the optimization level */
    FShaderCompileRequestBuilder &Optimize ( EShaderOptimizationLevel InLevel ) noexcept;

    /** @brief Enable or disable debug information */
    FShaderCompileRequestBuilder &Debug ( Bool bInEnable = true ) noexcept;

    /** @brief Set the GLSL version */
    FShaderCompileRequestBuilder &Version ( Int32 InVersion ) noexcept;

public:

    /** @brief Add a preprocessor macro */
    FShaderCompileRequestBuilder &Macro ( FString InName, FString InValue = "1" ) noexcept;

    /** @brief Add an include search path */
    FShaderCompileRequestBuilder &Include ( FString InSearchPath ) noexcept;

public:

    /** @brief Set the shader stage to vertex */
    FShaderCompileRequestBuilder &Vertex () noexcept;

    /** @brief Set the shader stage to fragment */
    FShaderCompileRequestBuilder &Fragment () noexcept;

    /** @brief Set the shader stage to compute */
    FShaderCompileRequestBuilder &Compute () noexcept;

public:

    /** Finalization */
    [[nodiscard]] struct FShaderCompileRequest Build () const noexcept;

private:

    FShaderCompileRequest Request;
};

} // namespace LumenEngine
