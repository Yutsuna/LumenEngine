/**
 * @file CompilerBase.inl
 * @brief Inline implementations for the TCompiler class template.
 */

#pragma once

#include "CompilerCore/CompilerBase.hpp"

#include "Container/File.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

template <typename TTraits, typename TDerived>
LumenEngine::Compiler::TCompiler<TTraits, TDerived>::TCompiler( typename TTraits::ConfigType InConfig )
    : Config( std::move( InConfig ) ), Cache( MakeUnique<TCompilerCache<TTraits>>( Config ) )
{
    /* Ctor */
}

template <typename TTraits, typename TDerived>
typename TTraits::ResultType LumenEngine::Compiler::TCompiler<TTraits, TDerived>::Compile ( const TRequest &InRequest ) noexcept
{
    TOptional<FString> SourceOpt = FIOFile::ReadAllText( TTraits::GetSourcePath( InRequest ) );

    if ( not SourceOpt.has_value() )
    {
        return TResult::Failure( static_cast<decltype( TResult::Error )>( ECompilerError::FileNotFound ) );
    }
    return CompileFromSource( *SourceOpt, InRequest );
}

template <typename TTraits, typename TDerived>
typename TTraits::ResultType LumenEngine::Compiler::TCompiler<TTraits, TDerived>::CompileFromSource ( const FStringView InSource, const TRequest &InRequest ) noexcept
{
    TDerived *Derived      = static_cast<TDerived *>( this );
    const FSourceHash Hash = Derived->ComputeHash( InSource, InRequest );

    TOptional<TCompiled> Cached = Cache->TryGet( Hash, InRequest );
    if ( Cached.has_value() )
    {
        Cached->bFromCache = true;

        FString Error;
        if ( Derived->TryReflect( *Cached, Error ) )
        {
            return TResult::Success( std::move( *Cached ) );
        }
    }

    TCompiled Compiled;
    FString ErrorLog;
    if ( Derived->CompileFresh( InSource, InRequest, Hash, Compiled, ErrorLog ) )
    {
        if ( Derived->TryReflect( Compiled, ErrorLog ) )
        {
            Cache->Put( Hash, InRequest, Compiled );
            return TResult::Success( std::move( Compiled ) );
        }
    }
    return TResult::Failure( static_cast<decltype( TResult::Error )>( ECompilerError::CompilationFailed ), ErrorLog );
}

template <typename TTraits, typename TDerived> LumenEngine::UInt64 LumenEngine::Compiler::TCompiler<TTraits, TDerived>::GetCacheHitCount () const noexcept
{
    return Cache->GetMemoryCache().GetStats().Hits;
}

template <typename TTraits, typename TDerived> const typename TTraits::ConfigType &LumenEngine::Compiler::TCompiler<TTraits, TDerived>::GetConfig () const noexcept
{
    return Config;
}
