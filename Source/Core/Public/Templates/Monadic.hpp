/**
 * @file Monadic.hpp
 * @brief Auto Monadic Mixin operations
 */

#pragma once

#include <CoreExport.hpp>
#include <Templates/MonadicDetails.hpp>

#include <concepts>
#include <utility>

namespace Lumen
{

namespace Concepts
{

    /**
     * @concept CMonadic
     * @brief Test the 3 monadic state
     */
    template <class Container>
    concept CMonadic = ( CHasValueMember<std::remove_cvref_t<Container>> or CBoolTestable<std::remove_cvref_t<Container>> ) and
                       requires( Container &&InContainer ) { *std::forward<Container>( InContainer ); };

    /**
     * @concept CTransformable
     * @brief Test the transformable state (T -> U).
     */
    template <class Self, class Func>
    concept CTransformable = CMonadic<Self> and std::invocable<Func, MonadicInternal::TValueRef<Self>> and
                             ( not std::is_void_v<MonadicInternal::TTransformResult<Self, Func>> ) and
                             requires { typename MonadicInternal::TTransformOut<Self, Func>; };

    /**
     * @concept CBindable
     * @brief Test the bindable state
     */
    template <class Self, class Func>
    concept CBindable = CMonadic<Self> and
                        ( MonadicInternal::CIsVoidValue<Self> ? std::invocable<Func>
                                                              : std::invocable<Func, MonadicInternal::TValueRef<Self>> ) and
                        CMonadic<MonadicInternal::TBindOut<Self, Func>>;

    /**
     * @concept CRecoverable
     * @brief Test the recoverable state
     */
    template <class Self, class Func>
    concept CRecoverable = CMonadic<Self> and std::constructible_from<MonadicInternal::TSelfType<Self>, Self> and
                           requires( Self &&Container, Func &&InFunc ) {
                               {
                                   MonadicInternal::InvokeRecover( std::forward<Self>( Container ), std::forward<Func>( InFunc ) )
                               } -> CSameAs<MonadicInternal::TSelfType<Self>>;
                           };
} // namespace Concepts

/**
 * @struct FMonadic
 * @brief Monadic mixin with 0 CRTP
 */
struct CORE_EXPORT FMonadic
{
public:

    /** @brief T -> U TOptional<T>.Transform(T->U) = TOptional<U> */
    template <class Self, class Func>
        requires Concepts::CTransformable<Self, Func>
    [[nodiscard]] constexpr auto Transform ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicInternal::Has( std::declval<const MonadicInternal::TSelfType<Self> &>() ) ) and
        noexcept( *std::declval<Self>() ) and
        noexcept( MonadicInternal::MakeValue<MonadicInternal::TTransformOut<Self, Func>>( std::declval<Func>(),
                                                                                          *std::declval<Self>() ) ) and
        noexcept( MonadicInternal::MakeFailure<MonadicInternal::TTransformOut<Self, Func>>( std::declval<Self>() ) ) and
        ( std::is_lvalue_reference_v<Self> or std::is_nothrow_move_constructible_v<MonadicInternal::TSelfType<Self>> ) )
        -> MonadicInternal::TTransformOut<Self, Func>
    {
        using Out = MonadicInternal::TTransformOut<Self, Func>;

        if ( MonadicInternal::Has( InSelf ) ) [[likely]]
        {
            if constexpr ( std::is_lvalue_reference_v<Self> )
            {
                return MonadicInternal::MakeValue<Out>( std::forward<Func>( InFunc ), *std::forward<Self>( InSelf ) );
            }
            else
            {
                Out Result = MonadicInternal::MakeValue<Out>( std::forward<Func>( InFunc ), *std::forward<Self>( InSelf ) );
                [[maybe_unused]] MonadicInternal::TSelfType<Self> Consumed = std::forward<Self>( InSelf );
                return Result;
            }
        }
        return MonadicInternal::MakeFailure<Out>( std::forward<Self>( InSelf ) );
    }

public:

    /** @brief T -> TContainer<U> */
    template <class Self, class Func>
        requires Concepts::CBindable<Self, Func>
    [[nodiscard]] constexpr auto AndThen ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicInternal::Has( std::declval<const MonadicInternal::TSelfType<Self> &>() ) ) and
        noexcept( *std::declval<Self>() ) and
        ( MonadicInternal::CIsVoidValue<Self> ? std::is_nothrow_invocable_v<Func>
                                              : std::is_nothrow_invocable_v<Func, MonadicInternal::TValueRef<Self>> ) and
        noexcept( MonadicInternal::MakeFailure<MonadicInternal::TBindOut<Self, Func>>( std::declval<Self>() ) ) )
        -> MonadicInternal::TBindOut<Self, Func>
    {
        using Out = MonadicInternal::TBindOut<Self, Func>;

        if ( MonadicInternal::Has( InSelf ) ) [[likely]]
        {
            if constexpr ( MonadicInternal::CIsVoidValue<Self> )
            {
                return std::invoke( std::forward<Func>( InFunc ) );
            }
            else
            {
                return std::invoke( std::forward<Func>( InFunc ), *std::forward<Self>( InSelf ) );
            }
        }
        return MonadicInternal::MakeFailure<Out>( std::forward<Self>( InSelf ) );
    }

public:

    /** @brief Func() -> TSelfType */
    template <class Self, class Func>
        requires Concepts::CRecoverable<Self, Func>
    [[nodiscard]] constexpr auto OrElse ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicInternal::Has( std::declval<const MonadicInternal::TSelfType<Self> &>() ) ) and
        std::is_nothrow_constructible_v<MonadicInternal::TSelfType<Self>, Self> and
        noexcept( MonadicInternal::InvokeRecover( std::declval<Self>(), std::declval<Func>() ) ) )
        -> MonadicInternal::TSelfType<Self>
    {
        if ( MonadicInternal::Has( InSelf ) ) [[likely]]
        {
            return std::forward<Self>( InSelf );
        }
        return MonadicInternal::InvokeRecover( std::forward<Self>( InSelf ), std::forward<Func>( InFunc ) );
    }

public:

    /** @brief Inspect the container, calling the given function if the container is not empty. */
    template <class Self, class Func>
        requires Concepts::CMonadic<Self> && std::invocable<Func, MonadicInternal::TConstElementRef<Self>>
    constexpr auto Inspect ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicInternal::Has( std::declval<const MonadicInternal::TSelfType<Self> &>() ) ) and
        std::is_nothrow_invocable_v<Func, MonadicInternal::TConstElementRef<Self>> and
        ( std::is_lvalue_reference_v<Self> or std::is_nothrow_move_constructible_v<MonadicInternal::TSelfType<Self>> ) )
        -> std::conditional_t<std::is_lvalue_reference_v<Self>, Self, MonadicInternal::TSelfType<Self>>
    {
        if ( MonadicInternal::Has( InSelf ) ) [[likely]]
        {
            std::invoke( std::forward<Func>( InFunc ),
                         static_cast<MonadicInternal::TConstElementRef<Self>>( *std::forward<Self>( InSelf ) ) );
        }
        return std::forward<Self>( InSelf );
    }

public:

    /** @brief ValueOr.  Pas disponible pour void (pas de valeur) — SFINAE silencieuse. */
    template <class Self, class U>
        requires Concepts::CMonadic<Self> and std::convertible_to<U, std::remove_cvref_t<MonadicInternal::TValueRef<Self>>> and
                 std::convertible_to<MonadicInternal::TValueRef<Self>, std::remove_cvref_t<MonadicInternal::TValueRef<Self>>>
    [[nodiscard]] constexpr auto ValueOr ( this Self &&InSelf, U &&Fallback ) noexcept(
        noexcept( MonadicInternal::Has( std::declval<const MonadicInternal::TSelfType<Self> &>() ) ) &&
        std::is_nothrow_convertible_v<MonadicInternal::TValueRef<Self>, std::remove_cvref_t<MonadicInternal::TValueRef<Self>>> and
        std::is_nothrow_convertible_v<U, std::remove_cvref_t<MonadicInternal::TValueRef<Self>>> )
        -> std::remove_cvref_t<MonadicInternal::TValueRef<Self>>
    {
        using R = std::remove_cvref_t<MonadicInternal::TValueRef<Self>>;

        if ( MonadicInternal::Has( InSelf ) ) [[likely]]
        {
            return static_cast<R>( *std::forward<Self>( InSelf ) );
        }
        return static_cast<R>( std::forward<U>( Fallback ) );
    }
};

} // namespace Lumen
