/**
 * @file LumenCompiler.cpp
 * @brief Implementation of LumenCompiler
 */

#include "LumenCompiler/LumenCompiler.hpp"

/**
 * Ctor
 */

LumenEngine::Compiler::FLumenCompiler::FLumenCompiler ( FLumenCompilerConfig InConfig ) noexcept : TCompiler( std::move( InConfig ) )
{
    ScratchBuffer.reserve( Config.ScratchBufferSize );
}

/**
 * Compile
 */

LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompiler::CompileAsset ( const FLumenCompileRequest &InRequest ) noexcept
{
    return TCompiler::Compile( InRequest );
}

LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompiler::CompileAssetFromSource ( FStringView InSource,
                                                                                                           const FLumenCompileRequest &InRequest ) noexcept
{
    return TCompiler::CompileFromSource( InSource, InRequest );
}
