/**
 * @file GlslangBackend.cpp
 * @brief Implementation of the internal GLSL to SPIR-V compilation backend.
 */

#include "Shader/Internal/GlslangBackend.hpp"

#include "Container/File.hpp"
#include "Container/UniquePtr.hpp"

#include "Logging/Logger.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <filesystem>
#include <format>
#include <mutex>

namespace LumenEngine
{

namespace Internal
{

    namespace
    {

        using FGlslangIncludeResult = glslang::TShader::Includer::IncludeResult;

        /**
         * @brief Loads the content of a file into a GLSL include result.
         * @param InPath The path to the file to load.
         * @return A pointer to the loaded include result.
         */
        [[nodiscard]] FGlslangIncludeResult *LoadFile ( const AnsiChar *InPath )
        {
            TUniquePtr<FString> FileContentPtr = MakeUnique<FString>();
            TOptional<FString> FileContentOpt  = FIOFile::ReadAllText( InPath );

            if ( not FileContentOpt.has_value() )
            {
                return new FGlslangIncludeResult( InPath, "Failed to read file", 19, nullptr );
            }

            *FileContentPtr         = std::move( FileContentOpt.value() );
            const AnsiChar *DataPtr = FileContentPtr->data();
            const USize DataSize    = FileContentPtr->size();
            void *UserData          = FileContentPtr.Release();

            LUMEN_LOG_INFO( LogShaderCompiler, "Loading include file '{}'", InPath );
            return new FGlslangIncludeResult( InPath, DataPtr, DataSize, UserData );
        }

        /**
         * @class FGlslangIncluder
         * @brief Custom includer for glslang that searches specified directories for included files.
         */
        class FGlslangIncluder final : public glslang::TShader::Includer
        {
        public:

            LUMEN_DISABLE_UBSAN explicit FGlslangIncluder( const TVector<FString> &InSearchPaths, const FString &InSourceDirectory ) noexcept
                : SearchPaths( InSearchPaths ), SourceDirectory( InSourceDirectory )
            {
                /* Ctor */
            }

        public:

            LUMEN_DISABLE_UBSAN FGlslangIncludeResult *includeLocal ( const AnsiChar *InHeaderName, const AnsiChar *InIncluderName, USize /* Depth */ ) override
            {
                try
                {
                    std::filesystem::path IncluderPath( InIncluderName != nullptr ? InIncluderName : SourceDirectory.c_str() );
                    std::filesystem::path LocalPath = IncluderPath.parent_path() / InHeaderName;

                    if ( FIOFile::Exists( LocalPath ) )
                    {
                        return LoadFile( LocalPath.string().c_str() );
                    }
                }
                catch ( const std::filesystem::filesystem_error & )
                {
                    return includeSystem( InHeaderName, InIncluderName, 0 );
                }
                return includeSystem( InHeaderName, InIncluderName, 0 );
            }

            LUMEN_DISABLE_UBSAN FGlslangIncludeResult *includeSystem ( const AnsiChar *InHeaderName, const AnsiChar * /*InIncluderName*/, USize /* Depth */ ) override
            {
                try
                {
                    for ( const FString &SearchPath : SearchPaths )
                    {
                        std::filesystem::path SystemPath = std::filesystem::path( SearchPath.c_str() ) / InHeaderName;

                        if ( std::filesystem::exists( SystemPath ) )
                        {
                            return LoadFile( SystemPath.string().c_str() );
                        }
                    }
                }
                catch ( const std::filesystem::filesystem_error &FileSystemError )
                {
                    LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while resolving include '{}': {}", InHeaderName, FileSystemError.what() );
                }
                return new FGlslangIncludeResult( InHeaderName, "File not found", 14, nullptr );
            }

            LUMEN_DISABLE_UBSAN void releaseInclude ( FGlslangIncludeResult *InIncludeResult ) override
            {
                if ( InIncludeResult != nullptr )
                {
                    if ( InIncludeResult->userData != nullptr )
                    {
                        delete static_cast<FString *>( InIncludeResult->userData );
                    }
                    delete InIncludeResult;
                }
            }

        private:

            const TVector<FString> &SearchPaths;
            const FString &SourceDirectory;
        };

        [[nodiscard]] EShLanguage MapStage ( const EShaderStage::Type InStage ) noexcept
        {
            switch ( InStage )
            {
            case EShaderStage::Vertex:
                return EShLangVertex;
            case EShaderStage::Fragment:
                return EShLangFragment;
            case EShaderStage::Compute:
                return EShLangCompute;
            case EShaderStage::Geometry:
                return EShLangGeometry;
            case EShaderStage::TessellationControl:
                return EShLangTessControl;
            case EShaderStage::TessellationEvaluation:
                return EShLangTessEvaluation;
            default:
                return EShLangVertex;
            }
        }

        std::once_flag GGlslangInitFlag;
        Bool GGlslangInitResult = false;

    } // namespace

} // namespace Internal

} // namespace LumenEngine

LUMEN_DISABLE_UBSAN LumenEngine::Bool LumenEngine::Internal::FGlslangBackend::Initialize () noexcept
{
    std::call_once( GGlslangInitFlag, [] { GGlslangInitResult = static_cast<Bool>( glslang::InitializeProcess() ); } );
    return GGlslangInitResult;
}

LUMEN_DISABLE_UBSAN void LumenEngine::Internal::FGlslangBackend::Finalize () noexcept
{
    if ( GGlslangInitResult )
    {
        glslang::FinalizeProcess();
    }
}

LUMEN_DISABLE_UBSAN LumenEngine::Bool
LumenEngine::Internal::FGlslangBackend::Compile ( FStringView InSource, const FShaderCompileRequest &InRequest, FSpirVBlob &OutSpirV, FString &OutErrorLog ) noexcept
{
    const EShLanguage Lang = MapStage( InRequest.Stage );
    glslang::TShader Shader( Lang );

    const AnsiChar *SourcePtr = InSource.data();
    const Int32 SourceLen     = static_cast<Int32>( InSource.size() );
    Shader.setStringsWithLengths( &SourcePtr, &SourceLen, 1 );

    FString Preamble;
    for ( const FShaderMacro &Macro : InRequest.Macros )
    {
        Preamble += std::format( "#define {} {}\n", Macro.Name, Macro.Value );
    }
    Shader.setPreamble( Preamble.c_str() );
    Shader.setEntryPoint( InRequest.EntryPoint.c_str() );

    FString SourceDir = ".";
    try
    {
        if ( not InRequest.SourcePath.empty() )
        {
            SourceDir = std::filesystem::path( InRequest.SourcePath.c_str() ).parent_path().string();
        }
    }
    catch ( const std::filesystem::filesystem_error &FileSystemError )
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while determining source directory for '{}': {}", InRequest.SourcePath.c_str(), FileSystemError.what() );
    }

    FGlslangIncluder Includer( InRequest.IncludeDirectories, SourceDir );

    Shader.setEnvInput( glslang::EShSourceGlsl, Lang, glslang::EShClientVulkan, 100 );
    Shader.setEnvClient( glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3 );
    Shader.setEnvTarget( glslang::EShTargetSpv, glslang::EShTargetSpv_1_6 );

    EShMessages Messages = static_cast<EShMessages>( EShMsgSpvRules | EShMsgVulkanRules );
    if ( InRequest.bEmitDebugInfo )
    {
        Messages = static_cast<EShMessages>( Messages | EShMsgDebugInfo );
    }

    if ( not Shader.parse( GetDefaultResources(), InRequest.GlslVersion, false, Messages, Includer ) )
    {
        OutErrorLog = Shader.getInfoLog();
        return false;
    }

    glslang::TProgram Program;
    Program.addShader( &Shader );
    if ( not Program.link( Messages ) )
    {
        OutErrorLog = Program.getInfoLog();
        return false;
    }

    glslang::SpvOptions SpvOptions;
    SpvOptions.generateDebugInfo = InRequest.bEmitDebugInfo;
    SpvOptions.disableOptimizer  = ( InRequest.Optimization == EShaderOptimizationLevel::None );
    SpvOptions.optimizeSize      = ( InRequest.Optimization == EShaderOptimizationLevel::Size );

    glslang::GlslangToSpv( *Program.getIntermediate( Lang ), OutSpirV, &SpvOptions );
    return true;
}
