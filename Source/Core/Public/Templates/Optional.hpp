/**
 * @file Optional.hpp
 * @brief TOptional type declaration
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Concepts/ConstructibleFrom.hpp>
#include <Concepts/ConvertibleTo.hpp>
#include <Concepts/SameAs.hpp>
#include <Templates/Invoke.hpp>
#include <Templates/Monadic.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace Lumen
{

/**
 * @class TOptional
 * @brief Optional type
 * @tparam ValueType Type of the optional value
 */
template <class ValueType> class TOptional : public FMonadic
{
public:

    template <class NewValueType> using Rebind = TOptional<NewValueType>;

public:

    /** @brief Default constructor */
    constexpr TOptional () noexcept : Dummy( 0 )
    {
        /** Empty */
    }

    /** @brief Copy constructor from value */
    constexpr explicit TOptional ( const ValueType &InValue ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> )
        : Value( InValue ), bHasValue( true )
    {
        /** Empty */
    }

    /** @brief Move constructor from value */
    constexpr explicit TOptional ( ValueType &&InValue ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        : Value( std::move( InValue ) ), bHasValue( true )
    {
        /** Empty */
    }

    /** @brief Emplace construction from a function */
    template <class Function, class Arguments>
    constexpr TOptional ( FEmplaceTag LUMEN_UNUSED InTag, Function &&Func, Arguments &&Args ) noexcept(
        std::is_nothrow_invocable_v<Function, Arguments> &&
        std::is_nothrow_constructible_v<ValueType, std::invoke_result_t<Function, Arguments>> )
        : Value( Invoke( std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ), bHasValue( true )
    {
        /** Empty */
    }

public:

    constexpr TOptional ( const TOptional & ) = default;

    /** @brief Copy constructor */
    constexpr TOptional ( const TOptional &InOptional ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> )
        requires( not std::is_trivially_copy_constructible_v<ValueType> && std::is_copy_constructible_v<ValueType> )
        : Dummy( 0 )
    {
        if ( InOptional.bHasValue )
        {
            std::construct_at( std::addressof( Value ), InOptional.Value );
            bHasValue = true;
        }
    }

    constexpr TOptional ( TOptional && ) = default;

    /** @brief Move constructor */
    constexpr TOptional ( TOptional &&InOptional ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        requires( not std::is_trivially_move_constructible_v<ValueType> && std::is_move_constructible_v<ValueType> )
        : Dummy( 0 )
    {
        if ( InOptional.bHasValue )
        {
            std::construct_at( std::addressof( Value ), std::move( InOptional.Value ) );
            bHasValue = true;
        }
    }

    /** @brief Converting copy constructor (implicit if ValueType is implicitly constructible from Other) */
    template <class Other>
        requires( not Concepts::CSameAs<Other, ValueType> ) && Concepts::CConstructibleFrom<ValueType, const Other &>
    constexpr explicit( not Concepts::CConvertibleTo<const Other &, ValueType> )
        TOptional( const TOptional<Other> &InOptional ) noexcept( std::is_nothrow_constructible_v<ValueType, const Other &> )
        : Dummy( 0 )
    {
        if ( InOptional.HasValue() )
        {
            std::construct_at( std::addressof( Value ), *InOptional );
            bHasValue = true;
        }
    }

    /** @brief Converting move constructor (implicit if ValueType is implicitly constructible from Other) */
    template <class Other>
        requires( not Concepts::CSameAs<Other, ValueType> ) && Concepts::CConstructibleFrom<ValueType, Other &&>
    constexpr explicit( not Concepts::CConvertibleTo<Other &&, ValueType> )
        TOptional( TOptional<Other> &&InOptional ) noexcept( std::is_nothrow_constructible_v<ValueType, Other &&> )
        : Dummy( 0 )
    {
        if ( InOptional.HasValue() )
        {
            std::construct_at( std::addressof( Value ), *std::move( InOptional ) );
            bHasValue = true;
        }
    }

    constexpr TOptional &operator=( const TOptional & ) = default;

    /** @brief Copy assignment operator */
    constexpr TOptional &operator=( const TOptional &InOptional ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> &&
                                                                            std::is_nothrow_copy_assignable_v<ValueType> )
        requires( not( std::is_trivially_copy_constructible_v<ValueType> && std::is_trivially_copy_assignable_v<ValueType> &&
                       std::is_trivially_destructible_v<ValueType> ) &&
                  std::is_copy_constructible_v<ValueType> && std::is_copy_assignable_v<ValueType> )
    {
        if ( this == std::addressof( InOptional ) )
        {
            return *this;
        }

        if ( bHasValue and InOptional.bHasValue )
        {
            Value = InOptional.Value;
        }
        else if ( bHasValue )
        {
            Reset();
        }
        else if ( InOptional.bHasValue )
        {
            std::construct_at( std::addressof( Value ), InOptional.Value );
            bHasValue = true;
        }
        return *this;
    }

    constexpr TOptional &operator=( TOptional && ) = default;

    /** @brief Move assignment operator */
    constexpr TOptional &operator=( TOptional &&InOptional ) noexcept( std::is_nothrow_move_constructible_v<ValueType> &&
                                                                       std::is_nothrow_move_assignable_v<ValueType> )
        requires( not( std::is_trivially_move_constructible_v<ValueType> && std::is_trivially_move_assignable_v<ValueType> &&
                       std::is_trivially_destructible_v<ValueType> ) &&
                  std::is_move_constructible_v<ValueType> && std::is_move_assignable_v<ValueType> )
    {
        if ( this == std::addressof( InOptional ) )
        {
            return *this;
        }

        if ( bHasValue and InOptional.bHasValue )
        {
            Value = std::move( InOptional.Value );
        }
        else if ( bHasValue )
        {
            Reset();
        }
        else if ( InOptional.bHasValue )
        {
            std::construct_at( std::addressof( Value ), std::move( InOptional.Value ) );
            bHasValue = true;
        }
        return *this;
    }

    constexpr ~TOptional () = default;

    /** @brief Destructor */
    constexpr ~TOptional () noexcept( std::is_nothrow_destructible_v<ValueType> )
        requires( not std::is_trivially_destructible_v<ValueType> )
    {
        Reset();
    }

public:

    /** @return True if has value otherwise false */
    [[nodiscard]] constexpr Bool HasValue () const noexcept
    {
        return bHasValue;
    }

    [[nodiscard]] constexpr explicit operator Bool () const noexcept
    {
        return bHasValue;
    }

    /** @brief Destroy the contained value, making the optional empty. */
    constexpr void Reset () noexcept( std::is_nothrow_destructible_v<ValueType> )
    {
        if ( bHasValue )
        {
            std::destroy_at( std::addressof( Value ) );
            bHasValue = false;
        }
    }

    /** @brief Emplace a value in place (destroys any existing value first). */
    template <class... Args>
    constexpr ValueType &Emplace ( Args &&...InArgs ) noexcept( std::is_nothrow_constructible_v<ValueType, Args...> )
    {
        Reset();
        std::construct_at( std::addressof( Value ), std::forward<Args>( InArgs )... );
        bHasValue = true;
        return Value;
    }

    /** @return Reference to value */
    constexpr decltype( auto ) operator*( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue );
        return ( std::forward<decltype( Self )>( Self ).Value );
    }

    constexpr auto *operator->( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue );
        return std::addressof( Self.Value );
    }

    /** @brief Monadic Hook */
    template <class Function, class Arguments>
    constexpr static TOptional MonadicEmplace ( Function &&Func, Arguments &&Args ) noexcept(
        noexcept( TOptional( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ) )
    {
        return TOptional( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) );
    }

    /** @brief Releases the value after an rvalue monadic result has been constructed. */
    constexpr void MonadicConsume () noexcept( std::is_nothrow_destructible_v<ValueType> )
    {
        Reset();
    }

private:

    union
    {
        ValueType Value;
        Byte Dummy;
    };

    Bool bHasValue = false;
};

template <typename Type> inline constexpr Bool IsOptionalV                  = false;
template <typename Type> inline constexpr Bool IsOptionalV<TOptional<Type>> = true;

#define LUMEN_OPTIONAL( Var, Expr )                                                                                              \
    auto &&Var##_lumenOptional_ = ( Expr );                                                                                      \
    if ( not Var##_lumenOptional_.HasValue() ) [[unlikely]]                                                                      \
    {                                                                                                                            \
        return {};                                                                                                               \
    }                                                                                                                            \
    [[maybe_unused]] auto &&Var = *std::forward<decltype( Var##_lumenOptional_ )>( Var##_lumenOptional_ )

#define LUMEN_TRY_OPTIONAL_ERROR( Expr )                                                                                         \
    do                                                                                                                           \
    {                                                                                                                            \
        auto &&_lumenOptError_ = ( Expr );                                                                                       \
        if ( _lumenOptError_.HasValue() ) [[unlikely]]                                                                           \
        {                                                                                                                        \
            return Lumen::MakeUnexpected( *std::forward<decltype( _lumenOptError_ )>( _lumenOptError_ ) );                       \
        }                                                                                                                        \
    } while ( 0 )

} // namespace Lumen
