/// @file Expected.hpp
/// @brief Wrapper for encapsulating explicit values and errors

#pragma once

#include <Templates/Monadic.hpp>
#include <Templates/Unexpected.hpp>

namespace Lumen
{

///
/// @class TExpected
/// @brief Wrapper for encapsulating explicit values and errors
/// @tparam ValueType The type of the value stored in the expected
/// @tparam ErrorType The type of the error stored in the expected
///
template <class ValueType, class ErrorType> class TExpected : public FMonadic
{
public:

    /// @brief Rebinds TExpected type to a new value type
    template <class NewValueType> using Rebind = TExpected<NewValueType, ErrorType>;

public:

    /// @brief Constructs TExpected with a value
    constexpr TExpected(const ValueType &InValue) noexcept : Val(InValue), bHasValue(true)
    {
        /// Empty
    }

    /// @brief Constructs TExpected with a value passed by rvalue reference
    constexpr TExpected(ValueType &&InValue) noexcept : Val(std::move(InValue)), bHasValue(true)
    {
        /// Empty
    }

    /// @brief Constructs TExpected with an Error
    constexpr explicit TExpected(FUnexpect, const ErrorType &InError) noexcept : Err(InError), bHasValue(false)
    {
        /// Empty
    }

    /// @brief Constructs TExpected with an Error passed by rvalue reference
    constexpr explicit TExpected(FUnexpect, ErrorType &&InError) noexcept : Err(std::move(InError)), bHasValue(false)
    {
        /// Empty
    }

    /// @brief Constructs TExpected with a TUnexpected
    template <class OtherErrorType>
        requires std::is_constructible_v<ErrorType, OtherErrorType>
    constexpr TExpected(TUnexpected<OtherErrorType> InUnexpected) noexcept
        : Err(std::move(InUnexpected).Value()), bHasValue(false)
    {
        /// Empty
    }

public:

    /// @brief Destructor
    constexpr ~TExpected() noexcept
    {
        Destroy();
    }

public:

    /// @brief Copy constructor
    constexpr explicit TExpected(const TExpected &Other) noexcept : bHasValue(Other.bHasValue)
    {
        if (bHasValue)
        {
            std::construct_at(std::addressof(Val), Other.Val);
        }
        else
        {
            std::construct_at(std::addressof(Err), Other.Err);
        }
    }

    /// @brief Move constructor
    constexpr TExpected(TExpected &&Other) noexcept( std::is_nothrow_move_constructible_v<ValueType> &&
                                                     std::is_nothrow_move_constructible_v<ErrorType> )
        : bHasValue(Other.bHasValue)
    {
        if (bHasValue)
        {
            std::construct_at(std::addressof(Val), std::move(Other.Val));
        }
        else
        {
            std::construct_at(std::addressof(Err), std::move(Other.Err));
        }
    }

    /// @brief Copy assignment operator
    constexpr TExpected &operator=(const TExpected &Other) noexcept
    {
        if (this == std::addressof(Other))
        {
            return *this;
        }

        if (bHasValue and Other.bHasValue)
        {
            Val = Other.Val;
        }
        else if (!bHasValue and not Other.bHasValue)
        {
            Err = Other.Err;
        }
        else
        {
            Destroy();
            bHasValue = Other.bHasValue;
            if (bHasValue)
            {
                std::construct_at(std::addressof(Val), Other.Val);
            }
            else
            {
                std::construct_at(std::addressof(Err), Other.Err);
            }
        }
        return *this;
    }

    /// @brief Move assignment operator
    constexpr TExpected &operator=(TExpected &&Other) noexcept( std::is_nothrow_move_constructible_v<ValueType> &&
                                                                std::is_nothrow_move_assignable_v<ValueType> &&
                                                                std::is_nothrow_move_constructible_v<ErrorType> &&
                                                                std::is_nothrow_move_assignable_v<ErrorType> )
    {
        if (this == std::addressof(Other))
        {
            return *this;
        }

        if (bHasValue and Other.bHasValue)
        {
            Val = std::move(Other.Val);
        }
        else if (!bHasValue and not Other.bHasValue)
        {
            Err = std::move(Other.Err);
        }
        else
        {
            Destroy();
            bHasValue = Other.bHasValue;
            if (bHasValue)
            {
                std::construct_at(std::addressof(Val), std::move(Other.Val));
            }
            else
            {
                std::construct_at(std::addressof(Err), std::move(Other.Err));
            }
        }
        return *this;
    }

public:

    /// @brief Checks if the expected object contains a value
    [[nodiscard]] constexpr Bool HasValue() const noexcept
    {
        return bHasValue;
    }

    /// @brief Explicit conversion to Bool
    [[nodiscard]] constexpr explicit operator Bool() const noexcept
    {
        return bHasValue;
    }

    /// @brief Dereference operator
    constexpr decltype( auto ) operator*(this auto &&Self) noexcept
    {
        return (std::forward<decltype( Self )>(Self).Val);
    }

    /// @brief Arrow operator
    constexpr auto *operator->(this auto &&Self) noexcept
    {
        return std::addressof(std::forward<decltype( Self )>(Self).Val);
    }

    /// @brief Error accessor
    constexpr decltype( auto ) Error(this auto &&Self) noexcept
    {
        return (std::forward<decltype( Self )>(Self).Err);
    }

    /// @brief Monadic hook
    template <class Src> static constexpr TExpected MonadicFail(Src &&Failed)
    {
        return TExpected(FUnexpect{}, std::forward<Src>(Failed).Error());
    }

private:

    /// @brief Non-restricted union
    union
    {
        ValueType Val;
        ErrorType Err;
    };

    Bool bHasValue;

private:

    /// @brief Destroys the value or error stored in the expected
    constexpr void Destroy() noexcept
    {
        if (bHasValue)
        {
            std::destroy_at(std::addressof(Val));
        }
        else
        {
            std::destroy_at(std::addressof(Err));
        }
    }
};

///
/// @class TExpected<void, ErrorType>
/// @brief Specialization of TExpected for void value type
///
template <class ErrorType> class TExpected<void, ErrorType> : public FMonadic
{
public:

    template <class NewValueType> using Rebind = TExpected<NewValueType, ErrorType>;

public:

    /// @brief Default constructor representing success
    constexpr TExpected() noexcept : bHasValue(true)
    {
        /// Empty
    }

    constexpr explicit TExpected(FUnexpect, const ErrorType &InError) noexcept : Err(InError), bHasValue(false)
    {
        /// Empty
    }

    constexpr explicit TExpected(FUnexpect, ErrorType &&InError) noexcept : Err(std::move(InError)), bHasValue(false)
    {
        /// Empty
    }

    template <class OtherErrorType>
        requires std::is_constructible_v<ErrorType, OtherErrorType>
    constexpr TExpected(TUnexpected<OtherErrorType> InUnexpected) noexcept
        : Err(std::move(InUnexpected).Value()), bHasValue(false)
    {
        /// Empty
    }

    constexpr ~TExpected() noexcept
    {
        if (not bHasValue)
        {
            std::destroy_at(std::addressof(Err));
        }
    }

    constexpr TExpected(const TExpected &Other) noexcept : bHasValue(Other.bHasValue)
    {
        if (not bHasValue)
        {
            std::construct_at(std::addressof(Err), Other.Err);
        }
    }

    constexpr TExpected(TExpected &&Other) noexcept( std::is_nothrow_move_constructible_v<ErrorType> )
        : bHasValue(Other.bHasValue)
    {
        if (not bHasValue)
        {
            std::construct_at(std::addressof(Err), std::move(Other.Err));
        }
    }

    constexpr TExpected &operator=(const TExpected &Other) noexcept
    {
        if (this == std::addressof(Other))
        {
            return *this;
        }
        if (not bHasValue and not Other.bHasValue)
        {
            Err = Other.Err;
        }
        else if (not bHasValue and Other.bHasValue)
        {
            std::destroy_at(std::addressof(Err));
            bHasValue = true;
        }
        else if (bHasValue and not Other.bHasValue)
        {
            std::construct_at(std::addressof(Err), Other.Err);
            bHasValue = false;
        }
        return *this;
    }

    constexpr TExpected &operator=(TExpected &&Other) noexcept( std::is_nothrow_move_assignable_v<ErrorType> &&
                                                                std::is_nothrow_move_constructible_v<ErrorType> )
    {
        if (this == std::addressof(Other))
        {
            return *this;
        }
        if (not bHasValue and not Other.bHasValue)
        {
            Err = std::move(Other.Err);
        }
        else if (not bHasValue and Other.bHasValue)
        {
            std::destroy_at(std::addressof(Err));
            bHasValue = true;
        }
        else if (bHasValue and not Other.bHasValue)
        {
            std::construct_at(std::addressof(Err), std::move(Other.Err));
            bHasValue = false;
        }
        return *this;
    }

public:

    [[nodiscard]] constexpr Bool HasValue() const noexcept
    {
        return bHasValue;
    }

    [[nodiscard]] constexpr explicit operator Bool() const noexcept
    {
        return bHasValue;
    }

    constexpr void operator*() const noexcept
    {
        /// Empty
    }

    constexpr decltype( auto ) Error(this auto &&Self) noexcept
    {
        return (std::forward<decltype( Self )>(Self).Err);
    }

    template <class Src> static constexpr TExpected MonadicFail(Src &&Failed)
    {
        return TExpected(FUnexpect{}, std::forward<Src>(Failed).Error());
    }

private:

    union
    {
        ErrorType Err;
        Byte Dummy;
    };

    Bool bHasValue;
};

} // namespace Lumen