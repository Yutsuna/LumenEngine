/**
 * @file SharedPtr.hpp
 * @brief Declaration of smart pointer classes for memory management.
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"
#include "CoreTypes.hpp"

#include <type_traits>

namespace LumenEngine
{

template <typename Type> class TSharedRef;

template <typename Type> class TSharedPtr;

namespace SharedPtrInternal
{

    /**
     * @struct FReferenceController
     * @brief Thread-Safe reference controller for shared pointers.
     * @details Manages the shared reference count and the lifetime of the managed object.
     */
    struct FReferenceController
    {
        TAtomic<Int32> SharedCount{ 1 };

        virtual ~FReferenceController () = default;

        /** Destroys the managed object. */
        virtual void DestroyObject () = 0;

        /** Frees the memory block containing the controller. */
        virtual void Deallocate () = 0;
    };

    /**
     * @class TIntrusiveReferenceController
     * @brief Stores the object and the reference count in a single memory block.
     * @param Type The type of the managed object.
     */
    template <typename Type> class TIntrusiveReferenceController final : public FReferenceController
    {
    public:

        /** Properly aligned storage for the managed object. */
        alignas( Type ) Byte Storage[sizeof( Type )]{};

        /** Constructs the managed object in place with forwarded arguments. */
        template <typename... Arguments> explicit TIntrusiveReferenceController( Arguments &&...InArgs );

        /** Destroys the managed object. */
        void DestroyObject () override;

        /** Frees the memory block containing the controller. */
        void Deallocate () override;

        /** Returns a pointer to the managed object. */
        [[nodiscard]] Type *GetObjectPtr ();
    };

    /**
     * @class TDefaultReferenceController
     * @brief Controller for objects already allocated on the heap.
     * @param Type The type of the managed object.
     */
    template <typename Type> class TDefaultReferenceController final : public FReferenceController
    {
    public:

        /** Constructs the controller with an existing pointer. */
        explicit TDefaultReferenceController ( Type *InObject );

        /** Destroys the managed object via delete. */
        void DestroyObject () override;

        /** Frees the memory block containing the controller. */
        void Deallocate () override;

    private:

        Type *Object;
    };

    /**
     * @struct TRawPtrProxy
     * @brief Internal proxy used to allow implicit conversion from MakeShareable to Ref or Ptr.
     */
    template <typename Type> struct TRawPtrProxy
    {
        explicit TRawPtrProxy ( Type *InObject );

        Type *Object;
    };

    /**
     * @brief Creates a new TIntrusiveReferenceController instance with the
     * given arguments.
     * @param InArgs Arguments to forward to the constructor of ObjectType.
     * @return A pointer to the newly created TIntrusiveReferenceController.
     */
    template <typename ObjectType, typename... Arguments>
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *NewIntrusiveReferenceController ( Arguments &&...InArgs );

    /**
     * @brief Creates a new TDefaultReferenceController instance for the given
     * object.
     * @param InObject Pointer to the managed object.
     * @return A pointer to the newly created TDefaultReferenceController.
     */
    template <typename ObjectType> SharedPtrInternal::TDefaultReferenceController<ObjectType> *NewDefaultReferenceController ( ObjectType *InObject );

} // namespace SharedPtrInternal

/**
 * @class TSharedRef
 * @brief A non-nullable shared reference smart pointer.
 * @param Type The type of the managed object.
 */
template <typename Type> class TSharedRef
{
    static_assert( !std::is_void_v<Type>, "TSharedRef<void> is not supported. Use a base class pointer." );

public:

    /** Copy Constructor */
    TSharedRef ( const TSharedRef &Other );

    /** Move Constructor */
    TSharedRef ( TSharedRef &&Other ) noexcept;

    /** Proxy Constructor: Allows implicit conversion from MakeShareable to TSharedRef */
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedRef( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy );

    /** Upcast Constructor: Allows conversion from derived to base types. */
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedRef( const TSharedRef<OtherType> &Other );

    /** Destructor */
    ~TSharedRef ();

    /** Assignment Operators */
    TSharedRef &operator=( const TSharedRef &Other );
    TSharedRef &operator=( TSharedRef &&Other ) noexcept;

    /** Accessors */
    Type &operator*() const;
    Type *operator->() const;
    [[nodiscard]] Type *Get () const;

    /** Returns the current number of shared references. */
    [[nodiscard]] Int32 GetSharedReferenceCount () const;

public:

    /** Comparison Operators */
    template <typename OtherType> Bool operator==( const TSharedRef<OtherType> &Other ) const noexcept
    {
        return Object == Other.Object;
    }
    template <typename OtherType> Bool operator!=( const TSharedRef<OtherType> &Other ) const noexcept
    {
        return Object != Other.Object;
    }
    template <typename OtherType> Bool operator==( const TSharedPtr<OtherType> &Other ) const noexcept
    {
        return Object == Other.Get();
    }
    template <typename OtherType> Bool operator!=( const TSharedPtr<OtherType> &Other ) const noexcept
    {
        return Object != Other.Get();
    }

private:

    /** Private constructor called by MakeSharedRef */
    TSharedRef ( Type *InObject, SharedPtrInternal::FReferenceController *InController );

    /** Releases the current reference. */
    void Release ();

    Type *Object;
    SharedPtrInternal::FReferenceController *Controller;

    template <typename ObjectType> friend class TSharedRef;

    template <typename ObjectType> friend class TSharedPtr;

    template <typename ObjectType> friend TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController );

    template <typename CastToType, typename CastFromType> friend TSharedRef<CastToType> StaticCastSharedRef ( const TSharedRef<CastFromType> &InSharedRef );
};

/**
 * @class TSharedPtr
 * @brief A nullable shared pointer.
 * @param Type The type of the managed object.
 */
template <typename Type> class TSharedPtr
{
public:

    /** Default Constructor (Null) */
    TSharedPtr ();

    /** NullptrType Constructor */
    TSharedPtr ( NullptrType );

    /** Constructor from TSharedRef (Implicit) */
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedPtr( const TSharedRef<OtherType> &Other );

    /** Copy Constructor */
    TSharedPtr ( const TSharedPtr &Other );

    /** Move Constructor */
    TSharedPtr ( TSharedPtr &&Other ) noexcept;

    /** Proxy Constructor: Allows implicit conversion from MakeShareable to TSharedPtr */
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedPtr( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy );

    /** Destructor */
    ~TSharedPtr ();

    /** Assignment Operators */
    TSharedPtr &operator=( const TSharedPtr &Other );
    TSharedPtr &operator=( const TSharedRef<Type> &Other );

    /** Accessors */
    Type &operator*() const;
    Type *operator->() const;
    [[nodiscard]] Type *Get () const;

    /** Validity checks */
    [[nodiscard]] Bool IsValid () const;
    explicit operator Bool () const;

    /** Resets the pointer to null. */
    void Reset ();

public:

    /** Comparison Operators */
    template <typename OtherType> Bool operator==( const TSharedPtr<OtherType> &Other ) const noexcept
    {
        return Object == Other.Get();
    }
    template <typename OtherType> Bool operator!=( const TSharedPtr<OtherType> &Other ) const noexcept
    {
        return Object != Other.Get();
    }
    template <typename OtherType> Bool operator==( const TSharedRef<OtherType> &Other ) const noexcept
    {
        return Object == Other.Get();
    }
    template <typename OtherType> Bool operator!=( const TSharedRef<OtherType> &Other ) const noexcept
    {
        return Object != Other.Get();
    }

    Bool operator==( NullptrType ) const noexcept
    {
        return Object == nullptr;
    }
    Bool operator!=( NullptrType ) const noexcept
    {
        return Object != nullptr;
    }

private:

    /** Releases the current reference. */
    void Release ();

    Type *Object;
    SharedPtrInternal::FReferenceController *Controller;

    template <typename ObjectType> friend class TSharedPtr;

    template <typename CastToType, typename CastFromType> friend TSharedPtr<CastToType> StaticCastSharedPtr ( const TSharedPtr<CastFromType> &InSharedPtr );
};

/**
 * @brief Creates a TSharedRef from an existing object and reference controller.
 * @param InObject Pointer to the managed object.
 * @param InController Pointer to the reference controller.
 * @return A TSharedRef managing the given object.
 */
template <typename ObjectType> static inline TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController );

/**
 * @brief Creates a new TSharedRef instance with the given arguments.
 * @param InArgs Arguments to forward to the constructor of ObjectType.
 * @return A TSharedRef managing the newly created object.
 */
template <typename ObjectType, typename... Arguments> static inline TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs );

/**
 * @brief Creates a TRawPtrProxy for the given object pointer.
 * @param InObject Pointer to the managed object.
 * @return A TRawPtrProxy wrapping the given object pointer.
 */
template <typename ObjectType> static inline SharedPtrInternal::TRawPtrProxy<ObjectType> MakeShareable ( ObjectType *InObject );

/**
 * @brief Casts a TSharedRef to a different type (static_cast).
 * @param InSharedRef The shared reference to cast.
 * @return A new TSharedRef managing the same object but with the target type.
 */
template <typename CastToType, typename CastFromType> static inline TSharedRef<CastToType> StaticCastSharedRef ( TSharedRef<CastFromType> const &InSharedRef );

/**
 * @brief Casts a TSharedPtr to a different type (static_cast).
 * @param InSharedPtr The shared pointer to cast.
 * @return A new TSharedPtr managing the same object but with the target type.
 */
template <typename CastToType, typename CastFromType> static inline TSharedPtr<CastToType> StaticCastSharedPtr ( TSharedPtr<CastFromType> const &InSharedPtr );

} // namespace LumenEngine

#include "Inline/SharedPtr.inl"
#include "Inline/SharedPtrInternal.inl"
#include "Inline/SharedRef.inl"
