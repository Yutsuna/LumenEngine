/**
 * @file Expected.hpp
 * @brief Wrapper for encapsulating explicit values and errors
 */

#pragma once

#include <Templates/Invoke.hpp>
#include <Templates/Monadic.hpp>
#include <Templates/Unexpected.hpp>

#include <Concepts/ConstructibleFrom.hpp>
#include <Concepts/ConvertibleTo.hpp>
#include <Concepts/SameAs.hpp>
#include <Core/CoreDefinitions.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace Lumen
{

/**
 * @class TExpected
 * @brief Wrapper for encapsulating explicit values and errors
 * @tparam ValueType The type of the value stored in the expected
 * @tparam ErrorType The type of the error stored in the expected
 */
template <class ValueType, class ErrorType> class TExpected : public FMonadic
{
public:

    /** @brief Rebinds TExpected type to a new value type */
    template <class NewValueType> using Rebind = TExpected<NewValueType, ErrorType>;

public:

    /** @brief Constructs TExpected with a value */
    constexpr TExpected ( const ValueType &InValue ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> )
        : Val( InValue ), bHasValue( true )
    {
        /** Empty */
    }

    /** @brief Constructs TExpected with a value passed by rvalue reference */
    constexpr TExpected ( ValueType &&InValue ) noexcept( std::is_nothrow_move_constructible_v<ValueType> )
        : Val( std::move( InValue ) ), bHasValue( true )
    {
        /** Empty */
    }

    /** @brief Constructs TExpected with an Error */
    constexpr explicit TExpected ( FUnexpect LUMEN_UNUSED InPolicy,
                                   const ErrorType &InError ) noexcept( std::is_nothrow_copy_constructible_v<ErrorType> )
        : Err( InError ), bHasValue( false )
    {
        /** Empty */
    }

    /** @brief Constructs TExpected with an Error passed by rvalue reference */
    constexpr explicit TExpected ( FUnexpect LUMEN_UNUSED InPolicy,
                                   ErrorType &&InError ) noexcept( std::is_nothrow_move_constructible_v<ErrorType> )
        : Err( std::move( InError ) ), bHasValue( false )
    {
        /** Empty */
    }

    /** @brief Constructs TExpected from a TUnexpected lvalue (copies the error) */
    template <class OtherErrorType>
        requires Concepts::CConstructibleFrom<ErrorType, const OtherErrorType &>
    constexpr TExpected( const TUnexpected<OtherErrorType> &InUnexpected ) noexcept(
        std::is_nothrow_constructible_v<ErrorType, const OtherErrorType &> )
        : Err( InUnexpected.Value() ), bHasValue( false )
    {
        /** Empty */
    }

    /** @brief Constructs TExpected from a TUnexpected rvalue (moves the error) */
    template <class OtherErrorType>
        requires Concepts::CConstructibleFrom<ErrorType, OtherErrorType &&>
    constexpr TExpected( TUnexpected<OtherErrorType> &&InUnexpected ) noexcept(
        std::is_nothrow_constructible_v<ErrorType, OtherErrorType &&> )
        : Err( std::move( InUnexpected ).Value() ), bHasValue( false )
    {
        /** Empty */
    }

    /** @brief Constructs the callback result directly in value storage. */
    template <class Function, class Arguments>
    constexpr TExpected ( FEmplaceTag LUMEN_UNUSED InTag, Function &&Func, Arguments &&Args ) noexcept(
        std::is_nothrow_invocable_v<Function, Arguments> &&
        std::is_nothrow_constructible_v<ValueType, std::invoke_result_t<Function, Arguments>> )
        : Val( Invoke( std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ), bHasValue( true )
    {
        /** Empty */
    }

public:

    constexpr TExpected ( const TExpected & ) = default;

    /** @brief Copy constructor */
    constexpr TExpected ( const TExpected &Other ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> &&
                                                             std::is_nothrow_copy_constructible_v<ErrorType> )
        requires( not( std::is_trivially_copy_constructible_v<ValueType> && std::is_trivially_copy_constructible_v<ErrorType> ) &&
                  std::is_copy_constructible_v<ValueType> && std::is_copy_constructible_v<ErrorType> )
        : bHasValue( Other.bHasValue )
    {
        if ( bHasValue )
        {
            std::construct_at( std::addressof( Val ), Other.Val );
        }
        else
        {
            std::construct_at( std::addressof( Err ), Other.Err );
        }
    }

    constexpr TExpected ( TExpected && ) = default;

    /** @brief Move constructor */
    constexpr TExpected ( TExpected &&Other ) noexcept( std::is_nothrow_move_constructible_v<ValueType> &&
                                                        std::is_nothrow_move_constructible_v<ErrorType> )
        requires( not( std::is_trivially_move_constructible_v<ValueType> && std::is_trivially_move_constructible_v<ErrorType> ) &&
                  std::is_move_constructible_v<ValueType> && std::is_move_constructible_v<ErrorType> )
        : bHasValue( Other.bHasValue )
    {
        if ( bHasValue )
        {
            std::construct_at( std::addressof( Val ), std::move( Other.Val ) );
        }
        else
        {
            std::construct_at( std::addressof( Err ), std::move( Other.Err ) );
        }
    }

    /** @brief Constructs TExpected with a value convertible to ValueType (e.g. upcasting smart pointers). */
    template <class OtherValueType>
        requires( not Concepts::CSameAs<std::remove_cvref_t<OtherValueType>, TExpected> and
                  not IsUnexpectedV<std::remove_cvref_t<OtherValueType>> and
                  Concepts::CConstructibleFrom<ValueType, OtherValueType> )
    constexpr explicit( not Concepts::CConvertibleTo<OtherValueType, ValueType> )
        TExpected( OtherValueType &&InValue ) noexcept( std::is_nothrow_constructible_v<ValueType, OtherValueType> )
        : Val( std::forward<OtherValueType>( InValue ) ), bHasValue( true )
    {
        /** Empty */
    }

    constexpr TExpected &operator=( const TExpected & ) = default;

    /** @brief Copy assignment with a strong guarantee through a temporary and a non-throwing swap. */
    constexpr TExpected &operator=( const TExpected &Other ) noexcept( std::is_nothrow_copy_constructible_v<ValueType> &&
                                                                       std::is_nothrow_copy_constructible_v<ErrorType> )
        requires( not( std::is_trivially_copy_constructible_v<ValueType> && std::is_trivially_copy_assignable_v<ValueType> &&
                       std::is_trivially_destructible_v<ValueType> && std::is_trivially_copy_constructible_v<ErrorType> &&
                       std::is_trivially_copy_assignable_v<ErrorType> && std::is_trivially_destructible_v<ErrorType> ) &&
                  std::is_copy_constructible_v<ValueType> && std::is_copy_constructible_v<ErrorType> &&
                  std::is_copy_assignable_v<ValueType> && std::is_copy_assignable_v<ErrorType> &&
                  std::is_nothrow_move_constructible_v<ValueType> && std::is_nothrow_move_constructible_v<ErrorType> &&
                  std::is_nothrow_swappable_v<ValueType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ValueType> && std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this != std::addressof( Other ) )
        {
            TExpected Temporary( Other );
            Swap( Temporary );
        }
        return *this;
    }

    constexpr TExpected &operator=( TExpected && ) = default;

    /** @brief Move assignment with a strong guarantee for the destination. */
    constexpr TExpected &operator=( TExpected &&Other ) noexcept
        requires( not( std::is_trivially_move_constructible_v<ValueType> && std::is_trivially_move_assignable_v<ValueType> &&
                       std::is_trivially_destructible_v<ValueType> && std::is_trivially_move_constructible_v<ErrorType> &&
                       std::is_trivially_move_assignable_v<ErrorType> && std::is_trivially_destructible_v<ErrorType> ) &&
                  std::is_move_constructible_v<ValueType> && std::is_move_constructible_v<ErrorType> &&
                  std::is_move_assignable_v<ValueType> && std::is_move_assignable_v<ErrorType> &&
                  std::is_nothrow_move_constructible_v<ValueType> && std::is_nothrow_move_constructible_v<ErrorType> &&
                  std::is_nothrow_swappable_v<ValueType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ValueType> && std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this != std::addressof( Other ) )
        {
            TExpected Temporary( std::move( Other ) );
            Swap( Temporary );
        }
        return *this;
    }

    constexpr ~TExpected () = default;

    /** @brief Destructor */
    constexpr ~TExpected () noexcept
        requires( not( std::is_trivially_destructible_v<ValueType> && std::is_trivially_destructible_v<ErrorType> ) )
    {
        Destroy();
    }

public:

    /** @brief Exchanges both states and payloads without allocation or exception. */
    constexpr void Swap ( TExpected &Other ) noexcept
        requires( std::is_nothrow_move_constructible_v<ValueType> && std::is_nothrow_move_constructible_v<ErrorType> &&
                  std::is_nothrow_swappable_v<ValueType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ValueType> && std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this == std::addressof( Other ) )
        {
            return;
        }

        if ( bHasValue and Other.bHasValue )
        {
            using std::swap;
            swap( Val, Other.Val );
        }
        else if ( not bHasValue and not Other.bHasValue )
        {
            using std::swap;
            swap( Err, Other.Err );
        }
        else if ( bHasValue )
        {
            ValueType Temporary( std::move( Val ) );
            std::destroy_at( std::addressof( Val ) );
            std::construct_at( std::addressof( Err ), std::move( Other.Err ) );
            std::destroy_at( std::addressof( Other.Err ) );
            std::construct_at( std::addressof( Other.Val ), std::move( Temporary ) );
        }
        else
        {
            ErrorType Temporary( std::move( Err ) );
            std::destroy_at( std::addressof( Err ) );
            std::construct_at( std::addressof( Val ), std::move( Other.Val ) );
            std::destroy_at( std::addressof( Other.Val ) );
            std::construct_at( std::addressof( Other.Err ), std::move( Temporary ) );
        }

        if ( bHasValue != Other.bHasValue )
        {
            bHasValue       = not bHasValue;
            Other.bHasValue = not Other.bHasValue;
        }
    }

    /** @brief Checks if the expected object contains a value */
    [[nodiscard]] constexpr Bool HasValue () const noexcept
    {
        return bHasValue;
    }

    /** @brief Explicit conversion to Bool */
    [[nodiscard]] constexpr explicit operator Bool () const noexcept
    {
        return bHasValue;
    }

    /** @brief Dereference operator. Undefined behavior if the expected holds an error. */
    constexpr decltype( auto ) operator*( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue );
        return ( std::forward<decltype( Self )>( Self ).Val );
    }

    /** @brief Arrow operator. Undefined behavior if the expected holds an error. */
    constexpr auto *operator->( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue );
        return std::addressof( Self.Val );
    }

    /** @brief Error accessor. Undefined behavior if the expected holds a value. */
    constexpr decltype( auto ) Error ( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue == false );
        return ( std::forward<decltype( Self )>( Self ).Err );
    }

    /** @brief Monadic hook */
    template <class Src>
    static constexpr TExpected
    MonadicFail ( Src &&Failed ) noexcept( noexcept( TExpected( FUnexpect{}, std::forward<Src>( Failed ).Error() ) ) )
    {
        return TExpected( FUnexpect{}, std::forward<Src>( Failed ).Error() );
    }

    /** @brief Monadic hook */
    template <class Function, class Arguments>
    static constexpr TExpected MonadicEmplace ( Function &&Func, Arguments &&Args ) noexcept(
        noexcept( TExpected( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ) )
    {
        return TExpected( FEmplaceTag{}, std::forward<Function>( Func ), std::forward<Arguments>( Args ) );
    }

private:

    union
    {
        ValueType Val;
        ErrorType Err;
    };

    Bool bHasValue;

private:

    /** @brief Destroys the value or error stored in the expected */
    constexpr void Destroy () noexcept
    {
        if ( bHasValue )
        {
            std::destroy_at( std::addressof( Val ) );
        }
        else
        {
            std::destroy_at( std::addressof( Err ) );
        }
    }
};

/**
 * @class TExpected<void, ErrorType>
 * @brief Specialization of TExpected for void value type
 */
template <class ErrorType> class TExpected<void, ErrorType> : public FMonadic
{
public:

    template <class NewValueType> using Rebind = TExpected<NewValueType, ErrorType>;

public:

    /** @brief Default constructor representing success */
    constexpr TExpected () noexcept : Dummy{}, bHasValue( true )
    {
        /** Empty */
    }

    constexpr explicit TExpected ( FUnexpect LUMEN_UNUSED InPolicy,
                                   const ErrorType &InError ) noexcept( std::is_nothrow_copy_constructible_v<ErrorType> )
        : Err( InError ), bHasValue( false )
    {
        /** Empty */
    }

    constexpr explicit TExpected ( FUnexpect LUMEN_UNUSED InPolicy,
                                   ErrorType &&InError ) noexcept( std::is_nothrow_move_constructible_v<ErrorType> )
        : Err( std::move( InError ) ), bHasValue( false )
    {
        /** Empty */
    }

    template <class OtherErrorType>
        requires Concepts::CConstructibleFrom<ErrorType, const OtherErrorType &>
    constexpr TExpected( const TUnexpected<OtherErrorType> &InUnexpected ) noexcept(
        std::is_nothrow_constructible_v<ErrorType, const OtherErrorType &> )
        : Err( InUnexpected.Value() ), bHasValue( false )
    {
        /** Empty */
    }

    template <class OtherErrorType>
        requires Concepts::CConstructibleFrom<ErrorType, OtherErrorType &&>
    constexpr TExpected( TUnexpected<OtherErrorType> &&InUnexpected ) noexcept(
        std::is_nothrow_constructible_v<ErrorType, OtherErrorType &&> )
        : Err( std::move( InUnexpected ).Value() ), bHasValue( false )
    {
        /** Empty */
    }

    constexpr TExpected ( const TExpected & ) = default;

    constexpr TExpected ( const TExpected &Other ) noexcept( std::is_nothrow_copy_constructible_v<ErrorType> )
        requires( not std::is_trivially_copy_constructible_v<ErrorType> && std::is_copy_constructible_v<ErrorType> )
        : Dummy{}, bHasValue( Other.bHasValue )
    {
        if ( not bHasValue )
        {
            std::construct_at( std::addressof( Err ), Other.Err );
        }
    }

    constexpr TExpected ( TExpected && ) = default;

    constexpr TExpected ( TExpected &&Other ) noexcept( std::is_nothrow_move_constructible_v<ErrorType> )
        requires( not std::is_trivially_move_constructible_v<ErrorType> && std::is_move_constructible_v<ErrorType> )
        : Dummy{}, bHasValue( Other.bHasValue )
    {
        if ( not bHasValue )
        {
            std::construct_at( std::addressof( Err ), std::move( Other.Err ) );
        }
    }

    constexpr TExpected &operator=( const TExpected & ) = default;

    constexpr TExpected &operator=( const TExpected &Other ) noexcept( std::is_nothrow_copy_constructible_v<ErrorType> )
        requires( not( std::is_trivially_copy_constructible_v<ErrorType> && std::is_trivially_copy_assignable_v<ErrorType> &&
                       std::is_trivially_destructible_v<ErrorType> ) &&
                  std::is_copy_constructible_v<ErrorType> && std::is_copy_assignable_v<ErrorType> &&
                  std::is_nothrow_move_constructible_v<ErrorType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this != std::addressof( Other ) )
        {
            TExpected Temporary( Other );
            Swap( Temporary );
        }
        return *this;
    }

    constexpr TExpected &operator=( TExpected && ) = default;

    constexpr TExpected &operator=( TExpected &&Other ) noexcept
        requires( not( std::is_trivially_move_constructible_v<ErrorType> && std::is_trivially_move_assignable_v<ErrorType> &&
                       std::is_trivially_destructible_v<ErrorType> ) &&
                  std::is_move_constructible_v<ErrorType> && std::is_move_assignable_v<ErrorType> &&
                  std::is_nothrow_move_constructible_v<ErrorType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this != std::addressof( Other ) )
        {
            TExpected Temporary( std::move( Other ) );
            Swap( Temporary );
        }
        return *this;
    }

    constexpr ~TExpected () = default;

    constexpr ~TExpected () noexcept
        requires( not std::is_trivially_destructible_v<ErrorType> )
    {
        if ( not bHasValue )
        {
            std::destroy_at( std::addressof( Err ) );
        }
    }

public:

    constexpr void Swap ( TExpected &Other ) noexcept
        requires( std::is_nothrow_move_constructible_v<ErrorType> && std::is_nothrow_swappable_v<ErrorType> &&
                  std::is_nothrow_destructible_v<ErrorType> )
    {
        if ( this == std::addressof( Other ) )
        {
            return;
        }

        if ( not bHasValue and not Other.bHasValue )
        {
            using std::swap;
            swap( Err, Other.Err );
        }
        else if ( not bHasValue )
        {
            ErrorType Temporary( std::move( Err ) );
            std::destroy_at( std::addressof( Err ) );
            bHasValue = true;
            std::construct_at( std::addressof( Other.Err ), std::move( Temporary ) );
            Other.bHasValue = false;
        }
        else if ( not Other.bHasValue )
        {
            ErrorType Temporary( std::move( Other.Err ) );
            std::destroy_at( std::addressof( Other.Err ) );
            Other.bHasValue = true;
            std::construct_at( std::addressof( Err ), std::move( Temporary ) );
            bHasValue = false;
        }
    }

    [[nodiscard]] constexpr Bool HasValue () const noexcept
    {
        return bHasValue;
    }

    [[nodiscard]] constexpr explicit operator Bool () const noexcept
    {
        return bHasValue;
    }

    constexpr void operator*() const noexcept
    {
        LUMEN_ASSERT( bHasValue );
    }

    /** @brief Error accessor. Undefined behavior if the expected holds success. */
    constexpr decltype( auto ) Error ( this auto &&Self ) noexcept
    {
        LUMEN_ASSERT( Self.bHasValue == false );
        return ( std::forward<decltype( Self )>( Self ).Err );
    }

    template <class Src>
    static constexpr TExpected
    MonadicFail ( Src &&Failed ) noexcept( noexcept( TExpected( FUnexpect{}, std::forward<Src>( Failed ).Error() ) ) )
    {
        return TExpected( FUnexpect{}, std::forward<Src>( Failed ).Error() );
    }

private:

    union
    {
        ErrorType Err;
        Byte Dummy;
    };

    Bool bHasValue;
};

namespace Internal
{

    struct FExpectVoid final
    {
    };

    template <class TExpected> constexpr decltype( auto ) UnpackExpected ( TExpected &&Expected )
    {
        if constexpr ( Lumen::Concepts::CIsVoid<decltype( *Expected )> )
        {
            return FExpectVoid{};
        }
        else
        {
            return *std::forward<TExpected>( Expected );
        }
    }

} // namespace Internal

#define LUMEN_EXPECT( Var, Expr )                                                                                                \
    auto &&Var##_lumenExpected_ = ( Expr );                                                                                      \
    if ( not Var##_lumenExpected_ ) [[unlikely]]                                                                                 \
    {                                                                                                                            \
        return Lumen::MakeUnexpected( std::forward<decltype( Var##_lumenExpected_ )>( Var##_lumenExpected_ ).Error() );          \
    }                                                                                                                            \
    [[maybe_unused]] auto &&Var =                                                                                                \
        Lumen::Internal::UnpackExpected( std::forward<decltype( Var##_lumenExpected_ )>( Var##_lumenExpected_ ) )

#define LUMEN_TRY_EXPECT_ERROR( Expr )                                                                                           \
                                                                                                                                 \
    do                                                                                                                           \
    {                                                                                                                            \
        auto &&_lumenExpected_ = ( Expr );                                                                                       \
        if ( not _lumenExpected_ ) [[unlikely]]                                                                                  \
        {                                                                                                                        \
            return Lumen::MakeUnexpected( std::forward<decltype( _lumenExpected_ )>( _lumenExpected_ ).Error() );                \
        }                                                                                                                        \
    } while ( false )

} // namespace Lumen
