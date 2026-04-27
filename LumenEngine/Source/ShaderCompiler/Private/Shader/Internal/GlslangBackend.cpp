/**
 * @file GlslangBackend.cpp
 * @brief Implementation of the internal GLSL to SPIR-V compilation backend using glslang.
 */

#include "Shader/Internal/GlslangBackend.hpp"

#include "Container/File.hpp"
#include "Container/UniquePtr.hpp"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <filesystem>
#include <format>

namespace LumenEngine
{

namespace Internal
{

    namespace
    {
        using FGlslangIncludeResult = glslang::TShader::Includer::IncludeResult;

        /**
         * @brief Load a file's content for glslang's includer interface.
         * @param InPath The path to the file to load.
         * @return A pointer to an FGlslangIncludeResult containing the file's content or an
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

            return new FGlslangIncludeResult( InPath, DataPtr, DataSize, UserData );
        }

        /**
         * @class FGlslangIncluder
         * @brief GLSLang Includer Implementation that searches for included files in specified directories
         */
        class FGlslangIncluder final : public glslang::TShader::Includer
        {
        public:

            explicit FGlslangIncluder ( const TVector<FString> &InSearchPaths, const FString &InSourceDirectory ) noexcept
                : SearchPaths( InSearchPaths ), SourceDirectory( InSourceDirectory )
            {
                /* Ctor */
            }

        public:

            /**
             * @brief Handle local includes (e.g., #include "file.glsl")
             * @details Searching in the includer's directory first, then falling back to system include paths.
             * @param InHeaderName The name of the header being included.
             * @param InIncluderName The name of the file that is including the header (can be null).
             * @param Depth The depth of the include (not used in this implementation).
             * @return The File's content or an error result if the file cannot be found or read.
             */
            FGlslangIncludeResult *includeLocal ( const AnsiChar *InHeaderName, const AnsiChar *InIncluderName, USize /* Depth */ ) override
            {
                /** TODO: Modularize in FIOFile */
                std::filesystem::path IncluderPath( InIncluderName != nullptr ? InIncluderName : SourceDirectory.c_str() );
                std::filesystem::path LocalPath = IncluderPath.parent_path() / InHeaderName;

                if ( std::filesystem::exists( LocalPath ) )
                {
                    return LoadFile( LocalPath.string().c_str() );
                }
                return includeSystem( InHeaderName, InIncluderName, 0 );
            }

            /**
             * @brief Handle system includes (e.g., #include <file.glsl>)
             * @details Searching in the specified system include paths.
             * @param InHeaderName The name of the header being included.
             * @param InIncluderName The name of the file that is including the header
             * @param Depth The depth of the include (not used in this implementation).
             * @return The File's content or an error result if the file cannot be found or read.
             */
            FGlslangIncludeResult *includeSystem ( const AnsiChar *InHeaderName, const AnsiChar * /*InIncluderName*/, USize /* Depth */ ) override
            {
                for ( const FString &SearchPath : SearchPaths )
                {
                    std::filesystem::path SystemPath = std::filesystem::path( SearchPath.c_str() ) / InHeaderName;

                    if ( std::filesystem::exists( SystemPath ) )
                    {
                        return LoadFile( SystemPath.string().c_str() );
                    }
                }
                return new FGlslangIncludeResult( InHeaderName, "File not found", 14, nullptr );
            }

            /**
             * @brief Release the memory allocated for an included file.
             * @param InIncludeResult The include result to release.
             */
            void releaseInclude ( FGlslangIncludeResult *InIncludeResult ) override
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

        /**
         * @brief Map a shader stage to its corresponding GLSLang language.
         * @param InStage The shader stage to map.
         * @return The corresponding GLSLang language.
         */
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

    } // namespace

} // namespace Internal

} // namespace LumenEngine

LumenEngine::Bool LumenEngine::Internal::FGlslangBackend::Initialize () noexcept
{
    static const Bool bInit = static_cast<Bool>( glslang::InitializeProcess() );

    return bInit;
}

void LumenEngine::Internal::FGlslangBackend::Finalize () noexcept
{
    glslang::FinalizeProcess();
}

/** TODO: Modularize */
LumenEngine::Bool
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

    FString SourceDir;
    try
    {
        SourceDir = std::filesystem::path( InRequest.SourcePath.c_str() ).parent_path().string();
    }
    catch ( ... )
    {
        SourceDir = ".";
    }

    FGlslangIncluder Includer( InRequest.IncludeDirectories, SourceDir );

    const Int32 ClientInputSemanticsVersion                   = 100;
    const glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_3;
    const glslang::EShTargetLanguageVersion TargetVersion     = glslang::EShTargetSpv_1_6;

    Shader.setEnvInput( glslang::EShSourceGlsl, Lang, glslang::EShClientVulkan, ClientInputSemanticsVersion );
    Shader.setEnvClient( glslang::EShClientVulkan, VulkanClientVersion );
    Shader.setEnvTarget( glslang::EShTargetSpv, TargetVersion );

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
