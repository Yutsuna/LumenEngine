/**
 * @file Optional.hpp
 * @brief TOptional type declaration
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Templates/Monadic.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace Lumen
{

/**
 * @tag FEmplaceTag
 * @brief Emplace Tag
 */
struct FEmplaceTag
{
};

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
        /**
         *
         */
    }

    /** @brief Move constructor from value */
    constexpr explicit TOptional ( ValueType &&InValue ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        : Value( std::move( InValue ) ), bHasValue( true )
    {
        /**
         *
         */
    }

    /** @brief Emplace construction from a function */
    template <class Function, class Arguments>
    constexpr TOptional( FEmplaceTag,
                         Function &&Func,
                         Arguments &&Args ) noexcept( std::is_nothrow_invocable_v<Function, Arguments> )
        : Value( std::invoke( std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ), bHasValue( true )
    {
        /** Empty */
    }

public:

    /** @brief Copy constructor */
    constexpr TOptional ( const TOptional &InOptional ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> )
        requires std::copy_constructible<ValueType>
        : Dummy( 0 )
    {
        if ( InOptional.bHasValue )
        {
            std::construct_at( std::addressof( Value ), InOptional.Value );
            bHasValue = true;
        }
    }

    /** @brief Move constructor */
    constexpr TOptional ( TOptional &&InOptional ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        requires std::move_constructible<ValueType>
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
        requires( not std::same_as<Other, ValueType> ) && std::constructible_from<ValueType, const Other &>
    constexpr explicit( not std::convertible_to<const Other &, ValueType> )
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
        requires( not std::same_as<Other, ValueType> ) && std::constructible_from<ValueType, Other &&>
    constexpr explicit( not std::convertible_to<Other &&, ValueType> )
        TOptional( TOptional<Other> &&InOptional ) noexcept( std::is_nothrow_constructible_v<ValueType, Other &&> )
        : Dummy( 0 )
    {
        if ( InOptional.HasValue() )
        {
            std::construct_at( std::addressof( Value ), *std::move( InOptional ) );
            bHasValue = true;
        }
    }

    /** @brief Assignment operator deleted */
    TOptional &operator=( const TOptional & ) = delete;

public:

    /** @brief Destructor */
    constexpr ~TOptional () noexcept( std::is_nothrow_destructible_v<ValueType> )
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
        return ( std::forward<decltype( Self )>( Self ).Value );
    }

    /** @brief Monadic Hook */
    template <class Function, class Arguments>
    constexpr static TOptional MonadicEmplace ( Function &&Func,
                                                Arguments &&Args ) noexcept( std::is_nothrow_invocable_v<Function, Arguments> )
    {
        return TOptional( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) );
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

} // namespace Lumen
