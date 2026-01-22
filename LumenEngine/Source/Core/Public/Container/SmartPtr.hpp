/**
 * @file SmartPtr.hpp
 * @brief Declaration of smart pointer classes for memory management.
 */

#pragma once

#include "CoreTypes.hpp"
#include <type_traits>

namespace LumenEngine
{

template <typename Type>
class TSharedRef;

namespace SharedPtrInternal
{

    /**
     * @struct FReference
     * @brief Thread-Safe reference controller for shared pointers.
     * @details Manages the shared reference count and the lifetime of the managed object.
     */
    struct FReferenceController
    {
        Atomic<Int32> SharedCount{ 1 };

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
    template <typename Type>
    class TIntrusiveReferenceController final : public FReferenceController
    {
    public:

        /** Properly aligned storage for the managed object. */
        alignas( Type ) Byte Storage[sizeof( Type )];

        /** Constructs the managed object in place with forwarded arguments. */
        template <typename... Arguments>
        explicit TIntrusiveReferenceController( Arguments &&...InArgs );

        /** Destroys the managed object. */
        void DestroyObject () override;

        /** Frees the memory block containing the controller. */
        void Deallocate () override;

        /** Returns a pointer to the managed object. */
        Type *GetObjectPtr ();
    };

} // namespace SharedPtrInternal

/**
 * @class TSharedRef
 * @brief A non-nullable shared reference smart pointer.
 * @tparam Type The type of the managed object.
 */
template <typename Type>
class TSharedRef
{
    static_assert( !std::is_void_v<Type>, "TSharedRef<void> is not supported. Use a base class pointer." );

public:

    /** Copy Constructor */
    TSharedRef ( const TSharedRef &Other );

    /** Move Constructor */
    TSharedRef ( TSharedRef &&Other );

    /** Upcast Constructor: Allows conversion from derived to base types. */
    template <typename OtherType, typename = std::enable_if_t<std::is_convertible_v<OtherType *, Type *>>>
    TSharedRef( const TSharedRef<OtherType> &Other );

    /** Destructor */
    ~TSharedRef ();

    /** Assignment Operators */
    TSharedRef &operator=( const TSharedRef &Other );

    /** Accessors */
    Type &operator*() const;
    Type *operator->() const;
    Type *Get () const;

    /** Returns the current number of shared references. */
    Int32 GetSharedReferenceCount () const;

private:

    /** Private constructor called by MakeSharedRef */
    TSharedRef ( Type *InObject, SharedPtrInternal::FReferenceController *InController );

    /** Releases the current reference. */
    void Release ();

    Type                                    *Object;
    SharedPtrInternal::FReferenceController *Controller;

    template <typename ObjectType, typename... Arguments>
    friend TSharedRef<ObjectType> MakeSharedRef ( Arguments &&...InArgs );

    template <typename ObjectType>
    friend class TSharedRef;
};

/**
 * @brief Creates a TSharedRef from an existing object and reference controller.
 * @param InObject Pointer to the managed object.
 * @param InController Pointer to the reference controller.
 * @return A TSharedRef managing the given object.
 */
template <typename ObjectType>
static inline TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController );

/**
 * @brief Creates a new TSharedRef instance with the given arguments.
 * @param InArgs Arguments to forward to the constructor of ObjectType.
 * @return A TSharedRef managing the newly created object.
 */
template <typename ObjectType, typename... Arguments>
static inline TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs );

} // namespace LumenEngine
