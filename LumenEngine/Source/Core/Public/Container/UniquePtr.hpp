/**
 * @file UniquePtr.hpp
 * @brief Declaration of unique smart pointer class for exclusive memory ownership.
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"
#include "CoreTypes.hpp"

#include <cassert>

namespace LumenEngine
{

/**
 * @struct TDefaultDelete
 * @brief Default deleter functor used by TUniquePtr to destroy the managed object.
 * @param Type The type of the managed object.
 */
template <typename Type> struct TDefaultDelete
{
    /** Default Constructor */
    constexpr TDefaultDelete () noexcept = default;

    /** Converting Constructor: Allows upcasting deleters from derived to base. */
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TDefaultDelete( const TDefaultDelete<OtherType> & ) noexcept
    {
        /* Empty */
    }

    /** Deletes the provided pointer. */
    void operator()( Type *Ptr ) const
    {
        static_assert( sizeof( Type ) > 0, "Cannot delete an incomplete type." );
        delete Ptr;
    }
};

/**
 * @class TUniquePtr
 * @brief A smart pointer that retains sole ownership of an object through a pointer.
 * @param Type The type of the managed object.
 * @param Deleter The functor type used to destroy the managed object.
 */
template <typename Type, typename Deleter = TDefaultDelete<Type>> class TUniquePtr
{
public:

    /** Default Constructor (Null) */
    TUniquePtr ();

    /** NullptrType Constructor */
    TUniquePtr ( NullptrType );

    /** Explicit Constructor from a raw pointer */
    explicit TUniquePtr ( Type *InObject );

    /** Explicit Constructor from a raw pointer and a custom deleter */
    TUniquePtr ( Type *InObject, Deleter InDeleter );

    /** Copying is explicitly disabled to enforce unique ownership */
    TUniquePtr ( const TUniquePtr & )           = delete;
    TUniquePtr &operator=( const TUniquePtr & ) = delete;

    /** Move Constructor */
    TUniquePtr ( TUniquePtr &&Other ) noexcept;

    /** Converting Move Constructor: Allows upcasting from derived unique pointers. */
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /** Destructor */
    ~TUniquePtr ();

    /** Move Assignment Operator */
    TUniquePtr &operator=( TUniquePtr &&Other ) noexcept;

    /** Converting Move Assignment Operator */
    template <typename OtherType, typename OtherDeleter>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TUniquePtr &operator=( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept;

    /** Nullptr Assignment */
    TUniquePtr &operator=( NullptrType ) noexcept;

    /** Accessors */
    Type &operator*() const;
    Type *operator->() const;
    Type *Get () const;

    /** Deleter Accessors */
    Deleter &GetDeleter ();
    const Deleter &GetDeleter () const;

    /** Validity checks */
    bool IsValid () const;
    explicit operator bool () const;

    /**
     * @brief Relinquishes ownership of the managed object and returns the pointer.
     * @return The raw pointer to the previously managed object.
     */
    Type *Release ();

    /**
     * @brief Replaces the managed object with a new one, destroying the old one if it exists.
     * @param InObject The new object pointer to manage (can be null).
     */
    void Reset ( Type *InObject = nullptr );

private:

    Type *Object;

    /** C++20 attribute to ensure the deleter occupies no space if it is stateless */
    [[no_unique_address]] Deleter DeleterInstance;

    template <typename OtherType, typename OtherDeleter> friend class TUniquePtr;
};

/**
 * @brief Creates a new TUniquePtr instance with the given arguments.
 * @param InArgs Arguments to forward to the constructor of ObjectType.
 * @return A TUniquePtr managing the newly created object.
 */
template <typename ObjectType, typename... Arguments> static inline TUniquePtr<ObjectType> MakeUnique ( Arguments &&...InArgs );

} // namespace LumenEngine

#include "Inline/UniquePtr.inl"
