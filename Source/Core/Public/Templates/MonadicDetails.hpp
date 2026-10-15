/// @file MonadicDetails.hpp
/// @brief Monadic details implementation

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/ConvertibleTo.hpp>
#include <Concepts/SameAs.hpp>

#include <functional>

#pragma once

namespace Lumen
{

namespace Concepts
{
    template <class MonadicType>
    concept CHasValueMember = requires( const MonadicType &Container )
    ///
    {
        { Container.HasValue() } -> CConvertibleTo<Bool>;
    };

    template <class MonadicType>
    concept CBoolTestable = requires( const MonadicType &Container )
    ///
    { static_cast<Bool>( Container ); };

    template <class Self>
    concept CHasError = requires( Self &&Container )
    ///
    { std::forward<Self>( Container ).Error(); };

    template <class Out, class Source>
    concept CHasFailHook = requires
    ///
    {
        { Out::MonadicFail( std::declval<Source>() ) } -> CSameAs<Out>;
    };

    template <class Out, class Func, class Array>
    concept CHasEmplaceHook = requires
    ///
    {
        { Out::MonadicEmplace( std::declval<Func>(), std::declval<Array>() ) } -> CSameAs<Out>;
    };

} // namespace Concepts

namespace MonadicDetail
{

    template <class Self> using TSelfType        = std::remove_cvref_t<Self>;
    template <class Self> using TValueRef        = decltype( *std::declval<Self>() );
    template <class Self> using TConstElementRef = const std::remove_reference_t<TValueRef<Self>> &;

    template <class MonadicType>
    inline constexpr Bool bNothrowHas = []
    {
        if constexpr ( Concepts::CHasValueMember<MonadicType> )
        {
            return noexcept( static_cast<Bool>( std::declval<const MonadicType &>().HasValue() ) );
        }
        else
        {
            return noexcept( static_cast<Bool>( std::declval<const MonadicType &>() ) );
        }
    }();

    template <class MonadicType>
    [[nodiscard]] constexpr Bool Has ( const MonadicType &Container ) noexcept( bNothrowHas<MonadicType> )
    {
        if constexpr ( Concepts::CHasValueMember<MonadicType> )
        {
            return static_cast<Bool>( Container.HasValue() );
        }
        else
        {
            return static_cast<Bool>( Container );
        }
    }

    template <class Out, class Source>
    inline constexpr Bool bNothrowMakeFailure =
        []
    {
        if constexpr ( Concepts::CHasFailHook<Out, Source> )
        {
            return noexcept( Out::MonadicFail( std::declval<Source>() ) );
        }
        else
        {
            return std::is_nothrow_default_constructible_v<Out>;
        }
    }

    ();

    template <class Out, class Source>
    [[nodiscard]] constexpr Out MakeFailure ( Source &&Failed ) noexcept( bNothrowMakeFailure<Out, Source> )
    {
        if constexpr ( Concepts::CHasFailHook<Out, Source> )
        {
            return Out::MonadicFail( std::forward<Source>( Failed ) );
        }
        else
        {
            static_assert( not Concepts::CHasError<Source>,
                           "This container holds an error : define `static Out MonadicFail( Source && )` to propagate it." );
            return Out();
        }
    }

    template <class Out, class Func, class Array>
    inline constexpr Bool bNothrowMakeValue = []
    {
        if constexpr ( Concepts::CHasEmplaceHook<Out, Func, Array> )
        {
            return noexcept( Out::MonadicEmplace( std::declval<Func>(), std::declval<Array>() ) );
        }
        else
        {
            return std::is_nothrow_invocable_v<Func, Array> &&
                   std::is_nothrow_constructible_v<Out, std::invoke_result_t<Func, Array>>;
        }
    }();

    template <class Out, class Func, class Array>
    [[nodiscard]] constexpr Out MakeValue ( Func &&InFunc, Array &&Arg ) noexcept( bNothrowMakeValue<Out, Func, Array> )
    {
        if constexpr ( Concepts::CHasEmplaceHook<Out, Func, Array> )
        {
            return Out::MonadicEmplace( std::forward<Func>( InFunc ), std::forward<Array>( Arg ) );
        }
        else
        {
            return Out( std::invoke( std::forward<Func>( InFunc ), std::forward<Array>( Arg ) ) );
        }
    }

    /// @brief Transform a value into another value.
    template <class Self, class Func> using TTransformResult = std::remove_cv_t<std::invoke_result_t<Func, TValueRef<Self>>>;
    template <class Self, class Func> using TTransformOut    = TSelfType<Self>::template Rebind<TTransformResult<Self, Func>>;

    /// @brief AndThen : Bind a value into another value if it is valid.
    template <class Self, class Func> using TBindOut = std::remove_cvref_t<std::invoke_result_t<Func, TValueRef<Self>>>;

    /// @brief OrElse : f() or f(Error()) if the host has an error.
    template <class Self, class Func>
        requires Concepts::CHasError<Self>
    constexpr decltype( auto ) InvokeRecover ( Self &&Container, Func &&InFunc ) noexcept(
        std::is_nothrow_invocable_v<Func, decltype( std::forward<Self>( Container ).Error() )> )
    {
        return std::invoke( std::forward<Func>( InFunc ), std::forward<Self>( Container ).Error() );
    }

    template <class Self, class Func>
        requires( not Concepts::CHasError<Self> )
    constexpr decltype( auto ) InvokeRecover ( Self &&, Func &&InFunc ) noexcept( std::is_nothrow_invocable_v<Func> )
    {
        return std::invoke( std::forward<Func>( InFunc ) );
    }

} // namespace MonadicDetail

} // namespace Lumen