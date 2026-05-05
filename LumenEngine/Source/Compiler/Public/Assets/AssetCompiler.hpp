/**
 * @file AssetCompiler.hpp
 * @brief High-level API for compiling all project assets (Materials, Meshes, Shaders).
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"
#include "Container/Vector.hpp"

#include "LumenCompiler/LumenCompiler.hpp"
#include "ShaderCompiler/ShaderCompiler.hpp"

#include "Graphics/Resources/RenderMaterial.hpp"
#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

namespace Compiler
{

    class FCompilerHotReload;

    /**
     * @struct FAssetCompileResult
     * @brief Summary of a bulk compilation operation.
     */
    struct FAssetCompileResult
    {
        UInt32 SuccessCount = 0;
        UInt32 FailureCount = 0;
        TVector<FString> FailedFiles;

        [[nodiscard]] Bool IsSuccess () const noexcept;
    };

    /**
     * @class FAssetCompiler
     * @brief Orchestrates FLumenCompiler and FShaderCompiler to compile entire asset directories.
     */
    class LUMEN_ENGINE_API FAssetCompiler final
    {
    public:

        FAssetCompiler () noexcept;
        ~FAssetCompiler () noexcept;

    public:

        /**
         * @brief Initializes the compiler with an assets path and performs initial bulk compilation.
         * @param InAssetsPath Root path to the Assets/ folder.
         */
        void Initialize ( const FString &InAssetsPath ) noexcept;

        /**
         * @brief Updates the hot-reload watcher.
         */
        void Tick () noexcept;

    public:

        /**
         * @brief Loads and compiles (if needed) a mesh by its name.
         * @param InName The name of the mesh (e.g., "Triangle").
         * @return A ready-to-render mesh resource.
         */
        [[nodiscard]] TSharedPtr<Renderer::FRenderMesh> LoadMesh ( const FString &InName ) noexcept;

        /**
         * @brief Loads and compiles (if needed) a material by its name.
         * @param InName The name of the material (e.g., "Triangle").
         * @return A ready-to-use material resource.
         */
        [[nodiscard]] TSharedPtr<Renderer::FRenderMaterial> LoadMaterial ( const FString &InName ) noexcept;

    public:

        /**
         * @brief Recursively compiles all assets found in the specified root directory.
         * @param InAssetsPath Root path to the Assets/ folder.
         * @return A result summary.
         */
        [[nodiscard]] FAssetCompileResult CompileAll ( const FString &InAssetsPath ) noexcept;

        /**
         * @brief Specifically compiles a single folder of assets.
         * @param InFolderPath Path to the folder (e.g., "Assets/Materials").
         * @param InAssetType Expected asset type for files in this folder.
         * @return A result summary.
         */
        [[nodiscard]] FAssetCompileResult CompileFolder ( const FString &InFolderPath, EAssetType::Type InAssetType ) noexcept;

        /**
         * @brief Compiles a single asset file.
         * @param InFilePath Path to the file.
         * @param InAssetType The asset type (Shader, Mesh, or Material).
         * @return A result summary.
         */
        [[nodiscard]] FAssetCompileResult CompileFile ( const FString &InFilePath, EAssetType::Type InAssetType ) noexcept;

    private:

        void AssetOnReloadCallback ( const FString &InPath, const EAssetType::Type InType ) noexcept;

    private:

        FLumenCompiler LumenCompiler;
        FShaderCompiler ShaderCompiler;
        TUniquePtr<FCompilerHotReload> HotReload;

        FString AssetsPath;

        TMap<FString, TSharedPtr<Renderer::FRenderMesh>> MeshCache;
        TMap<FString, TSharedPtr<Renderer::FRenderMaterial>> MaterialCache;
        TMap<FString, TSharedPtr<Renderer::FRenderShader>> ShaderCache;
    };

} // namespace Compiler

} // namespace LumenEngine
