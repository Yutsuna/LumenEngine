/// @file Optional.hpp
/// @brief TOptional type declaration

#pragma once

#include <Core/CoreTypes.hpp>
#include <Templates/Monadic.hpp>

namespace Lumen
{

///
/// @tag FEmplaceTag
/// @brief Emplace Tag
///
struct FEmplaceTag
{
};

///
/// @class TOptional
/// @brief Optional type
/// @tparam ValueType Type of the optional value
///
template <class ValueType> class TOptional : public FMonadic
{
public:

    /// @brief Default constructor
    TOptional () noexcept : Value()
    {
        /// Empty
    }

    /// @brief Value constructor
    explicit TOptional ( ValueType InValue ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        : Value( std::move( InValue ) ), bHasValue( true )
    {
        /// Empty
    }

    /// @brief Emplace construction from a function
    template <class Function, class Arguments>
    TOptional( FEmplaceTag, Function &&Func, Arguments &&Args ) noexcept( std::is_nothrow_invocable_v<Function, Arguments> )
        : Value( std::invoke( std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ), bHasValue( true )
    {
        /// Empty
    }

public:

    /// @brief Copy constructor
    TOptional ( const TOptional &InOptional ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> )
        requires std::copy_constructible<ValueType>
    {
        if ( InOptional.bHasValue )
        {
            ::new ( &Value ) ValueType( InOptional.Value );
            bHasValue = true;
        }
    }

    /// @brief Move constructor
    TOptional ( TOptional &&InOptional ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        requires std::move_constructible<ValueType>
    {
        if ( InOptional.bHasValue )
        {
            ::new ( &Value ) ValueType( std::move( InOptional.Value ) );
            bHasValue = true;
        }
    }

    /// @brief Assignment operator deleted
    TOptional &operator=( const TOptional & ) = delete;

public:

    /// @brief Destructor
    ~TOptional () noexcept( std::is_nothrow_destructible_v<ValueType> )
    {
        if ( bHasValue )
        {
            Value.~ValueType();
        }
    }

public:

    /// @return True if has value otherwise false
    [[nodiscard]] Bool HasValue () const noexcept
    {
        return bHasValue;
    }

    /// @return Reference to value
    decltype( auto ) operator*( this auto &&Self ) noexcept
    {
        return std::forward_like<decltype( Self.Value )>( Self.Value );
    }

    /// @brief Monadic Hook
    template <class Function, class Arguments>
    static TOptional MonadicEmplace ( Function &&Func,
                                      Arguments &&Args ) noexcept( std::is_nothrow_invocable_v<Function, Arguments> )
    {
        return TOptional( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) );
    }

private:

    ValueType Value;
    Bool bHasValue = false;
};

} // namespace Lumen