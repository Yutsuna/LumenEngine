/**
 * @file ShaderCompilerTypes.hpp
 * @brief Declaration of shader compiler types.
 */

#pragma once

#include "Container/Function.hpp"
#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "CoreTypes.hpp"
#include <span>

namespace LumenEngine
{

using FSourceHash = UInt64;
using FSpirvWord  = UInt32;
using FSpirVBlob  = TVector<FSpirvWord>;

namespace EShaderStage
{

    enum Type : UInt8
    {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation,
        Count
    };

    /**
     * @brief Converts a shader stage type to its string representation.
     * @param InStage The shader stage type to convert.
     * @return A string representation of the shader stage type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InStage ) noexcept;

    /**
     * @brief Converts a shader stage type to its corresponding GLSL file extension.
     * @param InStage The shader stage type to convert.
     * @return A string representing the GLSL file extension for the shader stage type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToGlslExtension ( const Type InStage ) noexcept;

} // namespace EShaderStage

enum class EShaderOptimizationLevel : UInt8
{
    None        = 0, ///< -O0  — debug builds, full debug info preserved
    Size        = 1, ///< -Os  — minimize SPIR-V word count
    Performance = 2  ///< -O   — full performance optimization (default)
};

enum class EGlslProfile : UInt8
{
    Core,          ///< OpenGL Core Profile
    Compatibility, ///< OpenGL Compatibility Profile
    Es,            /// OpenGL ES
    Vulkan         ///< Maps to #version 460 + Vulkan semantics
};

namespace EShaderCompilerError
{

    enum Type : UInt8
    {
        None               = 0,
        FileNotFound       = 1,
        ReadFailed         = 2,
        CompilationFailed  = 3,
        OptimizationFailed = 4,
        CacheWriteFailed   = 5,
        CacheReadFailed    = 6,
        InvalidStage       = 7,
        InvalidSource      = 8,
        ReflectionFailed   = 9,
        GlslangInitFailed  = 10,
        UnsupportedProfile = 11,
    };

    /**
     * @brief Converts a shader compiler error type to its string representation.
     * @param InError The shader compiler error type to convert.
     * @return A string representation of the shader compiler error type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InError ) noexcept;

} // namespace EShaderCompilerError

/**
 * @struct FShaderMacro
 * @brief Represents a shader macro definition, consisting of a name and a value.
 * @example #define USE_PBR 1
 */
struct FShaderMacro
{
    FStringView Name;  ///< "USE_PBR"
    FStringView Value; ///< "1"

    [[nodiscard]] Bool operator==( const FShaderMacro &Other ) const noexcept;
};

/**
 * @struct FShaderCompileRequest
 */
struct FShaderCompileRequest
{
    /** Path to the shader source file to compile (e.g., "Shaders/PBR.frag") */
    FStringView SourcePath;

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
    TVector<FStringView> IncludeDirectories;

    /** When true, the compiler will emit debug information in the SPIR-V binary (e.g., OpLine instructions). */
    Bool bEmitDebugInfo = false;
};

/**
 * @struct FShaderResourceBinding
 * @brief One descriptor-set binding entry (from reflection)
 */
struct FShaderResourceBinding
{
    FString Name;
    UInt32 Set     = 0;
    UInt32 Binding = 0;

    /** Vulkan descriptor type as a string (e.g., "UniformBuffer", "SampledImage", ...) */
    FString DexcriptorType;

    /** Array element count ( 1 = non-array resource ) */
    UInt32 ArraySize = 1;

    /** The shader stage mask where this binding is active ( bit field over EShaderStage ) */
    UInt8 StageMask = 0;
};

/**
 * @struct FShaderVertexInput
 * @brief Represents a single vertex input attribute used by a vertex shader
 */
struct FShaderVertexInput
{
    FString Name;
    UInt32 Location = 0;
    FString TypeName;
};

/**
 * @struct FShaderResourceBinding
 * @brief Represents a single resource binding used by a shader
 */
struct FShaderPushConstantRange
{
    FString Name;
    UInt32 Offset = 0;
    UInt32 Size   = 0;
};

/**
 * @struct FShaderReflection
 * @brief Contains reflection data extracted from a compiled shader
 */
struct FShaderReflection
{
    /** List of resource bindings (uniforms, samplers, etc.) used by the shader. */
    TVector<FShaderResourceBinding> ResourceBindings;

    /** List of vertex input attributes (for vertex shaders) used by the shader. */
    TVector<FShaderVertexInput> VertexInputs;

    /** List of push constant ranges used by the shader (for Vulkan). */
    TVector<FShaderPushConstantRange> PushConstantRanges;

    /** Local workgroup size specified in the shader (for compute shaders). */
    UInt32 LocalSizeX = 0;
    UInt32 LocalSizeY = 0;
    UInt32 LocalSizeZ = 0;
};

/**
 * @struct FCompiledShader
 * @brief Represents a successfully compiled shader, including its SPIR-V binary and resource bindings.
 */
struct FCompiledShader
{
    /** The Raw Compiled SPIR-V binary data. */
    FSpirVBlob SpirV;

    /** Reflection data extracted from the compiled SPIR-V */
    FShaderReflection Reflection;

    /** The shader stage this compiled shader corresponds to. */
    EShaderStage::Type Stage = EShaderStage::Vertex;

    /** Hash that produced this binary */
    FSourceHash Hash = 0;

    /** True when loaded from disk cache */
    Bool bFromCache = false;

    /** The entry point function name used in the shader (default is "main"). */
    FString EntryPoint = "main";

    /** Byte size of the SPIR-V blob */
    [[nodiscard]] UInt64 GetByteSize () const noexcept;

    /** Returns true if the compiled shader contains valid SPIR-V code and reflection data. */
    [[nodiscard]] Bool IsValid () const noexcept;
};

/**
 * @struct FShaderCompileResult
 * @brief Success or Typed Error
 */
struct FShaderCompileResult
{
    /** The compiled shader */
    TOptional<FCompiledShader> Shader;

    /** If Shader is std::nullopt, this contains the error type. */
    EShaderCompilerError::Type Error = EShaderCompilerError::None;

    /** Human-readable diagnostic log */
    FString ErrorLog;

    /** Returns true if the compilation was successful */
    [[nodiscard]] Bool IsSuccess () const noexcept;

    /** Factory method for creating a successful compile result */
    [[nodiscard]] static FShaderCompileResult Success ( FCompiledShader &&InShader ) noexcept;

    /** Factory method for creating a failed compile result with an Error Type. */
    [[nodiscard]] static FShaderCompileResult Failure ( const EShaderCompilerError::Type InError, FString InLog = {} ) noexcept;
};

/**
 * @struct FShaderCacheMetaData
 * @brief Metadata associated with a compiled shader cache entry
 */
struct FShaderCacheMetaData
{
    /** Hash of the shader source code, used to verify cache validity. */
    FSourceHash SourceHash = 0;

    /** The shader stage (e.g., vertex, fragment) that this cache entry corresponds to. */
    EShaderStage::Type Stage = EShaderStage::Vertex;

    /** The GLSL profile used during compilation (e.g., Core, ES, Vulkan). */
    EShaderOptimizationLevel Optimization = EShaderOptimizationLevel::Performance;

    /** The timestamp (in nanoseconds since epoch) when the shader was compiled */
    UInt64 CompiledAtNs = 0;

    /** The number of SPIR-V words in the compiled shader binary. */
    UInt32 SpirVWordCount = 0;

    /** The entry point function name used in the shader (default is "main"). */
    FStringView EntryPoint = "main";

    /** LSHC = Lumen Shader Cache Header */
    static constexpr UInt32 MagicNumber = 0X4C534843;

    /** Version number for the shader cache metadata format. */
    static constexpr UInt32 Version = 2U;

    /** Size of the fixed header portion of the cache metadata (excluding the variable-length entry point string). */
    static constexpr UInt64 HeaderSize = 31U;

    /** Serialise to a flat Byte buffer ( little-endian, fixed-layout ) */
    [[nodiscard]] TVector<Byte> Serialize () const;

    /** Returns std::nullopt if the buffer is malformed or contains invalid data. */
    [[nodiscard]] static TOptional<FShaderCacheMetaData> Deserialize ( const std::span<const Byte> InBytes );
};

/**
 * @struct FShaderCompilerConfig
 * @brief Configuration settings for the shader compiler
 */
struct FShaderCompilerConfig
{
    /** Root directory where .spv + .meta cache files are written to and read from. */
    FStringView CacheDirectory = "Data/ShaderCache/";

    /** Maximum allowed SPIR-V words allowed before the compiler rejects the shader. */
    UInt64 MaxSpirVWords = 0; ///< 0 means No Limit

    /** Whether to enable GLSLang's HLSL-compatible relaxed Vulkan rules */
    Bool bRelaxedVulkanRules = false;

    /** When true, every compile request also writes an assembly (.spvasm)  file */
    Bool bDumpAssembly = false;

    /** Called on every info message from the compiler. */
    TFunction<void( FStringView InMessage )> InfoCallback;

    /** Called on every warning message from the compiler. */
    TFunction<void( FStringView InMessage )> WarningCallback;

    /** Called on every error message from the compiler. */
    TFunction<void( FStringView InMessage )> ErrorgCallback;
};

} // namespace LumenEngine

#include "Inline/ShaderCompilerTypes.inl"
