/// @file UniquePtr.hpp
/// @brief Declaration of Unique Smart Pointer class for exclusive memory ownership.

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <Concepts/ConvertibleTo.hpp>

#include <Templates/Monadic.hpp>

namespace Lumen
{
namespace UniquePtrInternal
{
    ///
    /// @struct TDefaultDelete
    /// @brief Default deleter functor used by TUniquePtr to destroy the managed object.
    /// @tparam Type The object type
    ///
    template <typename Type> struct TDefaultDelete
    {
        /// @brief Default constructor
        constexpr TDefaultDelete () noexcept = default;

        /// @brief Converting constructor: Allows upcasting deleters from derived to base.
        template <typename OtherType>
            requires Concepts::CConvertibleTo<OtherType *, Type *>
        TDefaultDelete( const TDefaultDelete<OtherType> & ) noexcept;

        /// @brief Deletes the provided pointer.
        void operator()( const Type *Ptr ) const noexcept;
    };

    ///
    /// @struct TDefaultDelete
    /// @brief Default deleter functor used by TUniquePtr to destroy the managed object.
    /// @tparam Type The object type
    template <typename Type> struct TDefaultDelete<Type[]>
    {
        /// @brief Default constructor
        constexpr TDefaultDelete () noexcept = default;

        /// @brief Deletes the provided pointer.
        void operator()( const Type *Ptr ) const noexcept;
    };
} // namespace UniquePtrInternal

///
/// @class TUniquePtr
/// @brief Unique pointer class that owns and manages a dynamically allocated object.
/// @tparam Type The object type
/// @tparam Deleter The functor type used to destroy the managed object.
///
template <typename Type, typename Deleter = UniquePtrInternal::TDefaultDelete<Type>>
class TUniquePtr final : public FNonCopyable, public FMonadic
{
public:

    template <class U> using Rebind = TUniquePtr<U, Deleter>;

    /// @brief Default constructor
    TUniquePtr () noexcept;

    /// @brief NullPtrType constructor
    TUniquePtr ( NullPtrType ) noexcept;

    /// @brief Explicit constructor from a raw pointer
    explicit TUniquePtr ( Type *Ptr ) noexcept;

    /// @brief Explicit constructor from a raw pointer and a custom deleter
    explicit TUniquePtr ( Type *Ptr, Deleter InDeleter ) noexcept;

public:

    /// @brief Move constructor
    TUniquePtr ( TUniquePtr &&Other ) noexcept;

    /// @brief Converting Move constructor: allows upcasting from derived unique pointers.
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /// @brief Move assignment operator
    TUniquePtr &operator=( TUniquePtr &&Other ) noexcept;

    /// @brief Converting move assignment operator
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr &operator=( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /// @brief NullPtrType Assignment
    TUniquePtr &operator=( NullPtrType ) noexcept;

public:

    /// @brief Accessors
    Type &operator*() const noexcept;
    Type *operator->() const noexcept;
    [[nodiscard]] Type *Get () const noexcept;

    /// @brief Deleter Accessors
    [[nodiscard]] Deleter &GetDeleter () noexcept;
    [[nodiscard]] const Deleter &GetDeleter () const noexcept;

    /// @brief Validity checks
    [[nodiscard]] Bool IsValid () const noexcept;
    explicit operator Bool () const noexcept;

    /// @brief Release ownership of the managed object and returns the pointer.
    /// @return The raw pointer to the previously managed object.
    Type *Release () noexcept;

    /// @brief Replaces the managed object with a new one, destroying the old one if it exists.
    /// @param InObject The new object to manage.
    void Reset ( Type *InObject = nullptr ) noexcept;

public:

    /// @brief Destructor
    ~TUniquePtr () noexcept;

public:

    template <class Function, class Arguments>
    static TUniquePtr MonadicEmplace ( Function &&Func,
                                       Arguments &&Args ) noexcept( std::is_nothrow_invocable_v<Function, Arguments> )
    {
        return TUniquePtr( new Type( std::invoke( std::forward<Function>( Func ), std::forward<Arguments>( Args ) ) ) );
    }

private:

    Type *Object;

    /// @brief C++20 attribute to ensure deleter occupies no space if it is stateless.
    [[no_unique_address]] Deleter DeleterInstance;

    template <typename OtherType, typename OtherDeleter> friend class TUniquePtr;
};

///
/// @class TUniquePtr
/// @brief Unique pointer class that owns and manages a dynamically allocated object.
/// @tparam Type The object type
/// @tparam Deleter The functor type used to destroy the managed object.
///
template <typename Type, typename Deleter> class TUniquePtr<Type[], Deleter> final : public FNonCopyable
{
public:

    /// @brief Default constructor
    TUniquePtr () noexcept;

    /// @brief NullPtrType constructor
    TUniquePtr ( NullPtrType ) noexcept;

    /// @brief Explicit constructor from a raw pointer
    explicit TUniquePtr ( Type *Ptr ) noexcept;

    /// @brief Explicit constructor from a raw pointer and a custom deleter
    explicit TUniquePtr ( Type *Ptr, Deleter InDeleter ) noexcept;

public:

    /// @brief Move constructor
    TUniquePtr ( TUniquePtr &&Other ) noexcept;

    /// @brief Converting Move constructor: allows upcasting from derived unique pointers.
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /// @brief Move assignment operator
    TUniquePtr &operator=( TUniquePtr &&Other ) noexcept;

    /// @brief Converting move assignment operator
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr &operator=( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /// @brief NullPtrType Assignment
    TUniquePtr &operator=( NullPtrType ) noexcept;

public:

    /// @brief Accessors
    Type &operator[]( const USize Index ) const noexcept;
    [[nodiscard]] Type *Get () const noexcept;

    /// @brief Deleter Accessors
    [[nodiscard]] Deleter &GetDeleter () noexcept;
    [[nodiscard]] const Deleter &GetDeleter () const noexcept;

    /// @brief Validity checks
    [[nodiscard]] Bool IsValid () const noexcept;
    explicit operator Bool () const noexcept;

    /// @brief Release ownership of the managed object and returns the pointer.
    /// @return The raw pointer to the previously managed object.
    Type *Release () noexcept;

    /// @brief Replaces the managed object with a new one, destroying the old one if it exists.
    /// @param InObject The new object to manage.
    void Reset ( Type *InObject = nullptr ) noexcept;

public:

    /// @brief Destructor
    ~TUniquePtr () noexcept;

private:

    Type *Object;

    /// @brief C++20 attribute to ensure deleter occupies no space if it is stateless.
    [[no_unique_address]] Deleter DeleterInstance;

    template <typename OtherType, typename OtherDeleter> friend class TUniquePtr;
};

/// @brief Creates a new TUniquePtr instance for a single object.
template <typename ObjectType, typename... Arguments>
    requires( not std::is_array_v<ObjectType> )
TUniquePtr<ObjectType> MakeUnique ( Arguments &&...InArgs ) noexcept;

/// @brief Creates a new TUniquePtr instance for an array.
template <typename ObjectType>
    requires std::is_array_v<ObjectType>
TUniquePtr<ObjectType> MakeUnique ( const USize Size ) noexcept;
} // namespace Lumen

#include "Inline/UniquePtrInternal.inl"