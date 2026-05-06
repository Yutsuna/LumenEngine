/**
 * @file CompilerTypes.hpp
 * @brief Core types and configurations for the generic Compiler pipeline.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/Function.hpp"
#include "Container/String.hpp"
#include "Logging/LoggingCategory.hpp"

#include "Filesystem/Path.hpp"

namespace LumenEngine
{

namespace Compiler
{

    extern const FLogCategory LogCompiler;

    namespace ECompilerError
    {

        enum Type : UInt8
        {
            None = 0,
            FileNotFound,
            ReadFailed,
            CompilationFailed,
            OptimizationFailed,
            CacheWriteFailed,
            CacheReadFailed,
            InvalidStage,
            InvalidSource,
            ReflectionFailed,
            BackendInitFailed,
            UnsupportedProfile,
            SyntaxError,
            SerializationFailed
        };

        [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InError ) noexcept;

    } // namespace ECompilerError

    /**
     * @struct FCompilerConfig
     * @brief Base configuration for any compiler instance.
     */
    struct FCompilerConfig
    {
        Filesystem::FPath CacheDirectory = "Data/Cache/";
        TFunction<void( FStringView InMessage )> InfoCallback;
        TFunction<void( FStringView InMessage )> WarningCallback;
        TFunction<void( FStringView InMessage )> ErrorCallback;
    };

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/CompilerTypes.inl"
