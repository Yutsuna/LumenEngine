/**
 * @file LumenCompilerTypes.hpp
 * @brief Core types and configurations for the LumenCompiler asset pipeline.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "Version.hpp"

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

    using FAssetHash = UInt64;

    /**
     * @enum EAssetType
     * @brief Enumeration of supported asset types that can be compiled from DLSL source.
     */
    namespace EAssetType
    {

        enum Type : UInt32
        {
            Unknown  = 0,
            Mesh     = 1,
            Material = 2,
            Shader   = 3,
        };

        [[nodiscard]] constexpr const AnsiChar *ToString ( EAssetType::Type InAssetType ) noexcept;

    } // namespace EAssetType

    /**
     * @enum ELumenCompilerError
     * @brief Enumeration of possible error types that can occur during compilation.
     */
    namespace ELumenCompilerError
    {

        enum Type : UInt8
        {
            None                = 0,
            FileNotFound        = 1,
            ReadFailed          = 2,
            CompilationFailed   = 3,
            ParseFailed         = 4,
            SerializationFailed = 5,
            InvalidBlockType    = 6,
            BlockNotFound       = 7,
            AllocationFailed    = 8,
        };

        [[nodiscard]] constexpr const AnsiChar *ToString ( ELumenCompilerError::Type InError ) noexcept;

    } // namespace ELumenCompilerError

    /**
     * @struct FLumenBinaryHeader
     * @brief 16-byte aligned header for all .lumenbin files.
     */
    struct alignas( 16 ) FLumenBinaryHeader
    {
        UInt32 Magic               = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
        UInt32 FileVersion         = Version::Packed;
        EAssetType::Type AssetType = EAssetType::Unknown;
        UInt32 PayloadSize         = 0;
    };

    /**
     * @struct FLumenBinaryMeshHeader
     * @brief 32-byte header for compiled mesh assets.
     */
    struct alignas( 16 ) FLumenBinaryMeshHeader
    {
        UInt32 VertexCount  = 0;
        UInt32 IndexCount   = 0;
        UInt32 Topology     = 0;
        UInt32 CullMode     = 0;
        UInt32 WindingOrder = 0;
        UInt32 Pad[3]       = {};
    };

    /**
     * @struct FLumenBinaryMaterialHeader
     * @brief 32-byte header for compiled material assets.
     */
    struct alignas( 16 ) FLumenBinaryMaterialHeader
    {
        UInt32 BlendMode     = 0;
        UInt32 DepthTest     = 0;
        UInt32 DepthWrite    = 0;
        UInt32 CullMode      = 0;
        UInt32 WireFrame     = 0;
        UInt32 PropertyCount = 0;
        UInt32 Pad[2]        = {};
    };

    /**
     * @struct FLumenCompileRequest
     * @brief Describes a single DLSL asset compilation request.
     */
    struct FLumenCompileRequest
    {
        /** Path to the DLSL source file (e.g., "Assets/Hero.dlsl"). */
        FString SourcePath;

        /**
         * @brief Optional filter: compile only the root block with this name.
         *        If empty, the first block in the document is compiled.
         *        Example: "Hero" to extract @Mesh "Hero" { ... }.
         */
        FString TargetBlockName;

        /**
         * @brief Expected block type ("Mesh", "Material", ...).
         *        Used to validate the targeted block and select the correct serializer.
         *        If empty, the block type is inferred from the source.
         */
        FString ExpectedBlockType;

        /** When true, the compiler logs verbose diagnostic information. */
        Bool bDebug = false;
    };

    /**
     * @struct FCompiledLumenAsset
     * @brief Represents a successfully compiled .lumenbin asset blob.
     */
    struct FCompiledLumenAsset
    {
        /** Raw binary payload ready to be written to disk or uploaded to GPU memory. */
        TVector<Byte> BinaryBlob;

        /** Asset type resolved during compilation. */
        EAssetType::Type AssetType = EAssetType::Unknown;

        /** FNV-1a 64-bit hash of the source input + request parameters. */
        FAssetHash Hash = 0;

        /** Name of the root block that was compiled (mirrors FLumenCompileRequest::TargetBlockName). */
        FString BlockName;

        /** True when this result was retrieved from the cache without recompilation. */
        Bool bFromCache = false;

        /** Returns the total byte size of the binary blob. */
        [[nodiscard]] inline UInt64 GetByteSize () const noexcept;

        /** Returns true if the blob is non-empty. */
        [[nodiscard]] inline Bool IsValid () const noexcept;
    };

    /**
     * @struct FLumenCompileResult
     * @brief Typed success/failure wrapper returned by FLumenCompiler.
     */
    struct FLumenCompileResult
    {
        /** Populated on success; std::nullopt on failure. */
        TOptional<FCompiledLumenAsset> Asset;

        /** Error type. ELumenCompilerError::None when successful. */
        ELumenCompilerError::Type Error = ELumenCompilerError::None;

        /** Human-readable diagnostic log (parse errors, serialization errors, etc.). */
        FString ErrorLog;

        /** Returns true if compilation succeeded and the asset is valid. */
        [[nodiscard]] Bool IsSuccess () const noexcept;

        /** Factory: create a successful result. */
        [[nodiscard]] static FLumenCompileResult Success ( FCompiledLumenAsset &&InAsset ) noexcept;

        /** Factory: create a failed result. */
        [[nodiscard]] static FLumenCompileResult Failure ( ELumenCompilerError::Type InError, FString InLog = {} ) noexcept;
    };

    /**
     * @struct FLumenCacheMetaData
     * @brief Metadata stored alongside each .lumenbin cache entry.
     *
     * Binary layout
     *   [4] Magic
     *   [4] Version
     *   [8] SourceHash
     *   [4] AssetType
     *   [8] CompiledAtNs
     *   [4] BlobByteCount
     *   [1] BlockNameLen
     *   [BlockNameLen] BlockName chars
     */
    struct FLumenCacheMetaData
    {
        FAssetHash SourceHash      = 0;
        EAssetType::Type AssetType = EAssetType::Unknown;
        UInt64 CompiledAtNs        = 0;
        UInt32 BlobByteCount       = 0;
        FString BlockName;

        /** LMAS = Lumen Asset Cache (matches LUMEN_ASSET_CACHE_MAGIC_NUMBER) */
        static constexpr UInt32 MagicNumber = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
        static constexpr UInt32 Version     = 1U;

        /** Fixed header byte-count (excluding variable-length BlockName). */
        static constexpr UInt64 HeaderSize = 4U + 4U + 8U + 4U + 8U + 4U + 1U; // = 33

        /** Serialise to a flat byte buffer (little-endian). */
        [[nodiscard]] TVector<Byte> Serialize () const;

        /**
         * @brief Deserialise from a byte span.
         * @return std::nullopt if the buffer is malformed or the magic/version are wrong.
         */
        [[nodiscard]] static TOptional<FLumenCacheMetaData> Deserialize ( std::span<const Byte> InBytes );
    };

    /**
     * @struct FLumenCompilerConfig
     * @brief Configuration for the FLumenCompiler instance.
     */
    struct FLumenCompilerConfig
    {
        /** Root directory where .lumenbin + .meta cache files are written. */
        Filesystem::FPath CacheDirectory = "Data/AssetCache/";

        /**
         * @brief Size in bytes of the linear (arena) allocator scratch buffer used per
         */
        USize ScratchBufferSize = 1024ULL * 1024ULL;

        /** When true, the compiler logs verbose parse/serialize diagnostics. */
        Bool bVerboseLogging = false;
    };

    extern const FLogCategory LogLumenCompiler;

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/LumenCompilerTypes.inl"
