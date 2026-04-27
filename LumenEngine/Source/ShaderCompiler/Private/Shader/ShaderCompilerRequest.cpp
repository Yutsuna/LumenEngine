/**
 * @file ShaderCompilerRequest.cpp
 * @brief Implementation of FShaderCompileRequest and its builder.
 */

#include "Shader/ShaderCompilerRequest.hpp"

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Path ( FString InPath ) noexcept
{
    Request.SourcePath = std::move( InPath );
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::EntryPoint ( FString InName ) noexcept
{
    Request.EntryPoint = std::move( InName );
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Stage ( EShaderStage::Type InStage ) noexcept
{
    Request.Stage = InStage;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Optimize ( EShaderOptimizationLevel InLevel ) noexcept
{
    Request.Optimization = InLevel;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Debug ( Bool bInEnable ) noexcept
{
    Request.bEmitDebugInfo = bInEnable;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Version ( Int32 InVersion ) noexcept
{
    Request.GlslVersion = InVersion;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Macro ( FString InName, FString InValue ) noexcept
{
    Request.Macros.push_back( { .Name = std::move( InName ), .Value = std::move( InValue ) } );
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Include ( FString InSearchPath ) noexcept
{
    Request.IncludeDirectories.push_back( std::move( InSearchPath ) );
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Vertex () noexcept
{
    Request.Stage = EShaderStage::Vertex;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Fragment () noexcept
{
    Request.Stage = EShaderStage::Fragment;
    return *this;
}

LumenEngine::FShaderCompileRequestBuilder &LumenEngine::FShaderCompileRequestBuilder::Compute () noexcept
{
    Request.Stage = EShaderStage::Compute;
    return *this;
}

LumenEngine::FShaderCompileRequest LumenEngine::FShaderCompileRequestBuilder::Build () const noexcept
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