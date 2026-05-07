/**
 * @file ShaderCompilerTypes.hpp
 * @brief Declaration of shader compiler types.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/Function.hpp"
#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "Filesystem/Path.hpp"

#include "Logging/LoggingCategory.hpp"

#include <span>

namespace LumenEngine
{

namespace Compiler
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
        FString Name;  ///< "USE_PBR"
        FString Value; ///< "1"

        /**
         * @brief Compares two shader macro definitions.
         * @param InOther Macro to compare against.
         * @return True when both name and value match.
         */
        [[nodiscard]] Bool operator==( const FShaderMacro &InOther ) const noexcept;
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
        FString DescriptorType;

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

        /**
         * @brief Returns the byte size of the SPIR-V blob.
         * @return Number of bytes occupied by SpirV.
         */
        [[nodiscard]] UInt64 GetByteSize () const noexcept;

        /**
         * @brief Returns whether this compiled shader is valid.
         * @return True when SPIR-V payload is non-empty and reflection payload is consistent.
         */
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

        /**
         * @brief Returns whether compilation succeeded.
         * @return True when Shader is set and Error is None.
         */
        [[nodiscard]] Bool IsSuccess () const noexcept;

        /**
         * @brief Creates a successful compile result.
         * @param InShader Compiled shader payload.
         * @return Success result containing InShader.
         */
        [[nodiscard]] static FShaderCompileResult Success ( FCompiledShader &&InShader ) noexcept;

        /**
         * @brief Creates a failed compile result.
         * @param InError Error category associated with the failure.
         * @param InLog Human-readable diagnostic message.
         * @return Failed result with error and log set.
         */
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
        FString EntryPoint = "main";

        /** LSHC = Lumen Shader Cache Header */
        static constexpr UInt32 MagicNumber = LUMEN_SHADER_CACHE_MAGIC_NUMBER;

        /** Version number for the shader cache metadata format. */
        static constexpr UInt32 Version = 2U;

        /** Size of the fixed header portion of the cache metadata (excluding the variable-length entry point string). */
        static constexpr UInt64 HeaderSize = 31U;

        /**
         * @brief Serialises metadata to a flat byte buffer.
         * @return Little-endian fixed-layout metadata bytes.
         */
        [[nodiscard]] TVector<Byte> Serialize () const;

        /**
         * @brief Deserialises metadata from raw bytes.
         * @param InBytes Input byte span to decode.
         * @return Parsed metadata, or std::nullopt when malformed.
         */
        [[nodiscard]] static TOptional<FShaderCacheMetaData> Deserialize ( const std::span<const Byte> InBytes );
    };

    /**
     * @struct FShaderCompilerConfig
     * @brief Configuration settings for the shader compiler
     */
    struct FShaderCompilerConfig
    {
        /** Root directory where .spv + .meta cache files are written to and read from. */
        Filesystem::FPath CacheDirectory = "Data/ShaderCache/";

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
        TFunction<void( FStringView InMessage )> ErrorCallback;
    };

    extern const FLogCategory LogShaderCompiler;

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/ShaderCompilerTypes.inl"
