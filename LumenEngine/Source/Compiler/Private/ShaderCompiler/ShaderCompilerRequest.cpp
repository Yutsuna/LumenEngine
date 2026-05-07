/**
 * @file ShaderCompilerRequest.cpp
 * @brief Implementation of FShaderCompileRequest and its builder.
 */

#include "ShaderCompiler/ShaderCompilerRequest.hpp"

namespace LumenEngine::Compiler
{

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Path ( FString InPath ) noexcept
{
    Request.SourcePath = std::move( InPath );
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::EntryPoint ( FString InName ) noexcept
{
    Request.EntryPoint = std::move( InName );
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Stage ( EShaderStage::Type InStage ) noexcept
{
    Request.Stage = InStage;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Optimize ( EShaderOptimizationLevel InLevel ) noexcept
{
    Request.Optimization = InLevel;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Debug ( Bool InbEnable ) noexcept
{
    Request.bEmitDebugInfo = InbEnable;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Version ( Int32 InVersion ) noexcept
{
    Request.GlslVersion = InVersion;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Macro ( FString InName, FString InValue ) noexcept
{
    Request.Macros.push_back( { .Name = std::move( InName ), .Value = std::move( InValue ) } );
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Include ( FString InSearchPath ) noexcept
{
    Request.IncludeDirectories.push_back( std::move( InSearchPath ) );
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Vertex () noexcept
{
    Request.Stage = EShaderStage::Vertex;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Fragment () noexcept
{
    Request.Stage = EShaderStage::Fragment;
    return *this;
}

FShaderCompileRequestBuilder &FShaderCompileRequestBuilder::Compute () noexcept
{
    Request.Stage = EShaderStage::Compute;
    return *this;
}

FShaderCompileRequest FShaderCompileRequestBuilder::Build () const noexcept
{
    return FShaderCompileRequest{ .SourcePath         = Request.SourcePath,
                                  .Stage              = Request.Stage,
                                  .Profile            = Request.Profile,
                                  .Optimization       = Request.Optimization,
                                  .Macros             = Request.Macros,
                                  .EntryPoint         = Request.EntryPoint,
                                  .GlslVersion        = Request.GlslVersion,
                                  .IncludeDirectories = Request.IncludeDirectories,
                                  .bEmitDebugInfo     = Request.bEmitDebugInfo };
}

} // namespace LumenEngine::Compiler