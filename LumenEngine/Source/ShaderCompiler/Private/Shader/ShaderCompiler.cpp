/**
 * @file ShaderCompiler.cpp
 * @brief Implementation of FShaderCompiler class
 */

#include "Shader/ShaderCompiler.hpp"

#include "Container/File.hpp"
#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"
#include "CoreTypes.hpp"
#include "HAL/SharedMutex.hpp"
#include "Shader/ShaderCompilerTypes.hpp"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/resource_limits_c.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/SpvTools.h>

#include <mutex>
#include <optional>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <filesystem>

namespace LumenEngine
{

namespace
{

    using FGlslangIncludeResult = glslang::TShader::Includer::IncludeResult;

    /**
     * @brief Loads the content of a file into a string.
     * @param InPath The path to the file to load.
     * @return A pointer to the loaded include result.
     */
    [[nodiscard]] FGlslangIncludeResult *LoadFile ( const AnsiChar *InPath )
    {
        TUniquePtr<FString> FileContentPtr = MakeUnique<FString>( InPath );
        TOptional<FString> FileContentOpt  = FIOFile::ReadAllText( InPath );

        if ( not FileContentOpt.has_value() )
        {
            return new FGlslangIncludeResult( InPath, "Failed to read file", 14, nullptr );
        }

        *FileContentPtr         = std::move( FileContentOpt.value() );
        const AnsiChar *DataPtr = FileContentPtr->data();
        const USize DataSize    = FileContentPtr->size();

        void *UserData = FileContentPtr.Release();

        return new FGlslangIncludeResult( InPath, DataPtr, DataSize, UserData );
    }

    /**
     * @class FGlslangIncluder
     * @brief A custom includer for handling GLSL include directives.
     */
    class FGlslangIncluder final : public glslang::TShader::Includer
    {
    public:

        explicit FGlslangIncluder ( const TVector<FStringView> &InSearchPaths, const FStringView &InSourceDirectory ) noexcept
            : SearchPaths( InSearchPaths ), SourceDirectory( InSourceDirectory )
        {
            /* Ctor */
        }

    public:

        /**
         * @brief Includes a local header file.
         * @param InHeaderName The name of the header file to include.
         * @param InIncluderName The name of the file that includes the header.
         * @param InDepth The depth of the include.
         * @return A pointer to the included result.
         */
        FGlslangIncludeResult *includeLocal ( const AnsiChar *InHeaderName, const AnsiChar *InIncluderName, USize /* InDepth */ ) override
        {
            std::filesystem::path IncluderDir = std::filesystem::path( InIncluderName ).parent_path();

            if ( IncluderDir.empty() )
            {
                IncluderDir = SourceDirectory;
            }

            const std::filesystem::path LocalPath = IncluderDir / InHeaderName;

            if ( std::filesystem::exists( LocalPath ) )
            {
                return LoadFile( LocalPath.c_str() );
            }

            return includeSystem( InHeaderName, InIncluderName, 0 );
        }

        /**
         * @brief Includes a system header file.
         * @param InHeaderName The name of the header file to include.
         * @param InIncluderName The name of the file that includes the header.
         * @param InDepth The depth of the include.
         * @return A pointer to the included result.
         */
        FGlslangIncludeResult *includeSystem ( const AnsiChar *InHeaderName, const AnsiChar * /* InIncluderName */, USize /* InDepth */ ) override
        {
            for ( const FStringView &SearchPath : SearchPaths )
            {
                const std::filesystem::path SystemPath = std::filesystem::path( SearchPath ) / InHeaderName;

                if ( std::filesystem::exists( SystemPath ) )
                {
                    return LoadFile( SystemPath.c_str() );
                }
            }

            return new FGlslangIncludeResult( InHeaderName, "File not found", 14, nullptr );
        }

        /**
         * @brief Releases the memory allocated for an included result.
         * @param InIncludeResult The include result to release.
         */
        void releaseInclude ( FGlslangIncludeResult *InIncludeResult ) override
        {
            if ( InIncludeResult != nullptr and InIncludeResult->userData != nullptr )
            {
                delete static_cast<FString *>( InIncludeResult->userData );
            }
            delete InIncludeResult;
        }

    private:

        const TVector<FStringView> &SearchPaths;
        const FStringView SourceDirectory;

    }; // class FGlslangIncluder

    std::once_flag GGlslangInitOnceFlag;
    Bool GGlslangInitResult = false;

    void EnsureGlslangInitialized () noexcept
    {
        std::call_once( GGlslangInitOnceFlag, [] -> void { GGlslangInitResult = glslang::InitializeProcess(); } );
    }

} // namespace

} // namespace LumenEngine

/**
 * Ctor & Dtor
 */

LumenEngine::FShaderCompiler::FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept : Config( std::move( InConfig ) ), bInitialised( GGlslangInitResult )
{
    EnsureGlslangInitialized();

    if ( not bInitialised )
    {
        Config.ErrorCallback( "FShaderCompiler: glslang::InitializeProcess() failed" );
    }

    std::error_code ErrorCode;
    std::filesystem::create_directories( Config.CacheDirectory, ErrorCode );

    if ( ErrorCode and Config.ErrorCallback )
    {
        Config.ErrorCallback( std::format( "FShaderCompiler: Could not create cache directory '{}': {}", Config.CacheDirectory.c_str(), ErrorCode.message() ) );
    }
}

LumenEngine::FShaderCompiler::~FShaderCompiler () noexcept
{
    if ( bInitialised )
    {
        glslang::FinalizeProcess();
    }
}

/**
 * Public Methods
 */

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::CompileShader ( const FShaderCompileRequest &InRequest )
{
    FShaderCompileResult Result;

    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "FShaderCompiler was not initialised successfully" );
    }
    if ( InRequest.Stage >= EShaderStage::Count )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::InvalidStage, std::format( "Invalid shader stage: {}", static_cast<Int32>( InRequest.Stage ) ) );
    }

    TOptional<FString> SourceCodeOpt = FIOFile::ReadAllText( InRequest.SourcePath );
    if ( not SourceCodeOpt.has_value() )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::FileNotFound, std::format( "Failed to read shader source file: {}", InRequest.SourcePath ) );
    }

    return CompileShaderFromSource( *SourceCodeOpt, InRequest );
}

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest )
{
    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "FShaderCompiler was not initialised successfully" );
    }

    if ( InSource.empty() )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::InvalidSource, "Shader source code is empty" );
    }

    const FSourceHash ComputeHash = ComputeRequestHash( InSource, InRequest );

    {
        TSharedLock<FSharedMutex> ReadLock( CacheMutex );

        if ( const auto It = MemoryCache.find( ComputeHash ); It != MemoryCache.end() )
        {
            ++CacheHitCount;

            if ( Config.InfoCallback )
            {
                Config.InfoCallback( std::format( "FShaderCompiler: Cache HIT (memory) '{}' hash={:016X}", InRequest.SourcePath.string(), ComputeHash ) );
            }

            FCompiledShader CachedShader = It->second;
            CachedShader.bFromCache      = true;
            return FShaderCompileResult::Success( std::move( CachedShader ) );
        }
    }

    if ( TOptional<FCompiledShader> DiskShaderOpt = TryLoadFromDiskCache( ComputeHash, InRequest.Stage, InRequest.EntryPoint ) )
    {
        ++CacheHitCount;

        if ( Config.InfoCallback )
        {
            Config.InfoCallback( std::format( "FShaderCompiler: Cache HIT (disk) '{}' hash={:016X}", InRequest.SourcePath.string(), ComputeHash ) );
        }

        FCompiledShader DiskShader = DiskShaderOpt.value();
        DiskShader.bFromCache      = true;

        {
            TUniqueLock<FSharedMutex> WriteLock( CacheMutex );
            MemoryCache.emplace( ComputeHash, DiskShader );
        }

        return FShaderCompileResult::Success( std::move( DiskShader ) );
    }

    ++CacheMissCount;

    if ( Config.InfoCallback )
    {
        Config.InfoCallback( std::format( "FShaderCompiler: Cache MISS '{}' hash={:016X}", InRequest.SourcePath.string(), ComputeHash ) );
    }

    return CompileAndCacheShader( InSource, InRequest, ComputeHash );
}

/**
 * Private Methods
 */

namespace LumenEngine
{

namespace
{

    inline std::filesystem::path BuildCacheFilePath ( const std::filesystem::path &CacheDirectory, const FSourceHash Hash, const EShaderStage::Type Stage )
    {
        return CacheDirectory / std::format( "{:016X}_{}.spv", Hash, EShaderStage::ToString( Stage ) );
    }

    inline std::filesystem::path BuildMetaFilePath ( const std::filesystem::path &CacheDirectory, const FSourceHash Hash, const EShaderStage::Type Stage )
    {
        return CacheDirectory / std::format( "{:016X}_{}.meta", Hash, EShaderStage::ToString( Stage ) );
    }

} // namespace

} // namespace LumenEngine

/**
 * @brief Attempts to load a compiled shader from the disk cache.
 * @param InHash The hash of the shader source.
 * @param InStage The stage of the shader.
 * @param InEntryPoint The entry point of the shader.
 * @return An optional containing the compiled shader if found, otherwise an empty optional.
 */
LumenEngine::TOptional<LumenEngine::FCompiledShader>
LumenEngine::FCompiledShader::TryLoadFromDiskCache ( const FSourceHash InHash, const EShaderStage::Type InStage, const FStringView InEntryPoint )
{
    const std::filesystem::path SpvPath  = BuildCacheFilePath( Config.CacheDirectory, HashToHex( InHash ), InStage );
    const std::filesystem::path MetaPath = BuildMetaFilePath( Config.CacheDirectory, HashToHex( InHash ), InStage );

    if ( not std::filesystem::exists( SpvPath ) or not std::filesystem::exists( MetaPath ) )
    {
        return std::nullopt;
    }

    {
        TOptional<TVector<Byte>> MetaBytes = FIOFile::ReadAllBytes<Byte>( MetaPath.string() );
        if ( not MetaBytes.has_value() )
        {
            Config.ErrorCallback( std::format( "FShaderCompiler: Failed to read cache meta file '{}'", MetaPath.string() ) );
            return std::nullopt;
        }

        TOptional<FShaderCacheMetaData> MetaOpt = FShaderCacheMetaData::Deserialize( std::span<const Byte>( *MetaBytes ) );
        if ( not MetaOpt.has_value() )
        {
            Config.ErrorCallback( std::format( "FShaderCompiler: Failed to parse cache meta file '{}'", MetaPath.string() ) );
            return std::nullopt;
        }

        if ( MetaOpt->SourceHash != InHash )
        {
            Config.ErrorCallback(
                std::format( "FShaderCompiler: Cache meta file '{}' hash mismatch (expected {:016X}, got {:016X})", MetaPath.string(), InHash, MetaOpt->SourceHash ) );
            return std::nullopt;
        }

        if ( MetaOpt->Stage != InStage )
        {
            Config.ErrorCallback( std::format( "FShaderCompiler: Cache meta file '{}' stage mismatch (expected {}, got {})", MetaPath.string(),
                                               EShaderStage::ToString( InStage ), EShaderStage::ToString( MetaOpt->Stage ) ) );
            return std::nullopt;
        }
    }
}