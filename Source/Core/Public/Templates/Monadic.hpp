/// @file Monadic.hpp
/// @brief Auto Monadic Mixin operations

#pragma once

#include <Templates/MonadicDetails.hpp>

#include <concepts>
#include <functional>
#include <utility>

namespace Lumen
{

namespace Concepts
{

    /// @concept CMonadic
    /// @brief Test the 3 monadic state
    template <class Container>
    concept CMonadic = ( CHasValueMember<std::remove_cvref_t<Container>> or CBoolTestable<std::remove_cvref_t<Container>> ) and
                       requires( Container &&InContainer ) {
                           ///
                           *std::forward<Container>( InContainer );
                           ///
                       };

    /// @concept CTransformable
    /// @brief Test the transformable state
    template <class Self, class Func>
    concept CTransformable = CMonadic<Self> and std::invocable<Func, MonadicDetail::TValueRef<Self>> and
                             ( not std::is_void_v<MonadicDetail::TTransformResult<Self, Func>> ) and requires {
                                 ///
                                 typename MonadicDetail::TTransformOut<Self, Func>;
                                 ///
                             };

    /// @concept CBindable
    /// @brief Test the bindable state
    template <class Self, class Func>
    concept CBindable =
        CMonadic<Self> and std::invocable<Func, MonadicDetail::TValueRef<Self>> and CMonadic<MonadicDetail::TBindOut<Self, Func>>;

    /// @concept CRecoverable
    /// @brief Test the recoverable state
    template <class Self, class Func>
    concept CRecoverable = CMonadic<Self> and std::constructible_from<MonadicDetail::TSelfType<Self>, Self> and
                           requires( Self &&Container, Func &&InFunc ) {
                               ///
                               {
                                   MonadicDetail::InvokeRecover( std::forward<Self>( Container ), std::forward<Func>( InFunc ) )
                               } -> CSameAs<MonadicDetail::TSelfType<Self>>;
                               ///
                           };

} // namespace Concepts

///
/// @struct FMonadic
/// @brief Monadic mixin with 0 CRTP
///
struct FMonadic
{

public:

    /// @brief T -> U TOptional<T>.Transform(T->U) = TOptional<U>
    template <class Self, class Func>
        requires Concepts::CTransformable<Self, Func>
    [[nodiscard]] constexpr auto Transform ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicDetail::Has( std::declval<const MonadicDetail::TSelfType<Self> &>() ) ) and
        noexcept( *std::declval<Self>() ) and
        noexcept( MonadicDetail::MakeValue<MonadicDetail::TTransformOut<Self, Func>>( std::declval<Func>(),
                                                                                      *std::declval<Self>() ) ) and
        noexcept( MonadicDetail::MakeFailure<MonadicDetail::TTransformOut<Self, Func>>( std::declval<Self>() ) ) )
        -> MonadicDetail::TTransformOut<Self, Func>
    {
        using Out = MonadicDetail::TTransformOut<Self, Func>;

        if ( MonadicDetail::Has( InSelf ) ) [[likely]]
        {
            return MonadicDetail::MakeValue<Out>( std::forward<Func>( InFunc ), *std::forward<Self>( InSelf ) );
        }
        return MonadicDetail::MakeFailure<Out>( std::forward<Self>( InSelf ) );
    }

public:

    /// @brief T -> TContainer<U>
    template <class Self, class Func>
        requires Concepts::CBindable<Self, Func>
    [[nodiscard]] constexpr auto AndThen ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicDetail::Has( std::declval<const MonadicDetail::TSelfType<Self> &>() ) ) and
        noexcept( *std::declval<Self>() ) and std::is_nothrow_invocable_v<Func, MonadicDetail::TValueRef<Self>> and
        noexcept( MonadicDetail::MakeFailure<MonadicDetail::TBindOut<Self, Func>>( std::declval<Self>() ) ) )
        -> MonadicDetail::TBindOut<Self, Func>
    {
        using Out = MonadicDetail::TBindOut<Self, Func>;

        if ( MonadicDetail::Has( InSelf ) ) [[likely]]
        {
            return std::invoke( std::forward<Func>( InFunc ), *std::forward<Self>( InSelf ) );
        }
        return MonadicDetail::MakeFailure<Out>( std::forward<Self>( InSelf ) );
    }

public:

    /// @brief Func() -> TSelfType
    template <class Self, class Func>
        requires Concepts::CRecoverable<Self, Func>
    [[nodiscard]] constexpr auto OrElse ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicDetail::Has( std::declval<const MonadicDetail::TSelfType<Self> &>() ) ) and
        std::is_nothrow_constructible_v<MonadicDetail::TSelfType<Self>, Self> and
        noexcept( MonadicDetail::InvokeRecover( std::declval<Self>(), std::declval<Func>() ) ) ) -> MonadicDetail::TSelfType<Self>
    {
        if ( MonadicDetail::Has( InSelf ) ) [[likely]]
        {
            return std::forward<Self>( InSelf );
        }
        return MonadicDetail::InvokeRecover( std::forward<Self>( InSelf ), std::forward<Func>( InFunc ) );
    }

public:

    /// @brief Inspect the container, calling the given function if the container is not empty.
    template <class Self, class Func>
        requires Concepts::CMonadic<Self> && std::invocable<Func, MonadicDetail::TConstElementRef<Self>>
    constexpr auto Inspect ( this Self &&InSelf, Func &&InFunc ) noexcept(
        noexcept( MonadicDetail::Has( std::declval<const MonadicDetail::TSelfType<Self> &>() ) ) and
        std::is_nothrow_invocable_v<Func, MonadicDetail::TConstElementRef<Self>> and
        ( std::is_lvalue_reference_v<Self> or std::is_nothrow_move_constructible_v<MonadicDetail::TSelfType<Self>> ) )
        -> std::conditional_t<std::is_lvalue_reference_v<Self>, Self, MonadicDetail::TSelfType<Self>>
    {
        if ( MonadicDetail::Has( InSelf ) ) [[likely]]
        {
            std::invoke( std::forward<Func>( InFunc ), static_cast<MonadicDetail::TConstElementRef<Self>>( *InSelf ) );
        }
        return std::forward<Self>( InSelf );
    }

public:

    /// @brief Return the value of the container, or the fallback value if the container is empty.
    template <class Self, class U>
        requires Concepts::CMonadic<Self> and std::convertible_to<U, std::remove_cvref_t<MonadicDetail::TValueRef<Self>>> and
                 std::convertible_to<MonadicDetail::TValueRef<Self>, std::remove_cvref_t<MonadicDetail::TValueRef<Self>>>
    [[nodiscard]] constexpr auto ValueOr ( this Self &&InSelf, U &&Fallback ) noexcept(
        noexcept( MonadicDetail::Has( std::declval<const MonadicDetail::TSelfType<Self> &>() ) ) &&
        std::is_nothrow_convertible_v<MonadicDetail::TValueRef<Self>, std::remove_cvref_t<MonadicDetail::TValueRef<Self>>> and
        std::is_nothrow_convertible_v<U, std::remove_cvref_t<MonadicDetail::TValueRef<Self>>> )
        -> std::remove_cvref_t<MonadicDetail::TValueRef<Self>>
    {
        using R = std::remove_cvref_t<MonadicDetail::TValueRef<Self>>;

        if ( MonadicDetail::Has( InSelf ) ) [[likely]]
        {
            return static_cast<R>( *std::forward<Self>( InSelf ) );
        }
        return static_cast<R>( std::forward<U>( Fallback ) );
    }
};

} // namespace Lumen