/**
 * @file ShaderCompiler.cpp
 * @brief Implementation of FShaderCompiler class
 */

#include "Shader/ShaderCompiler.hpp"

#include "Container/File.hpp"
#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/resource_limits_c.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/SpvTools.h>

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

} // namespace

} // namespace LumenEngine