///
/// @file SharedPtr.hpp
/// @brief Shared Memory Pointer
///

#pragma once

#include <Concepts/ConvertibleTo.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{
template <typename Type> class TSharedRef;
template <typename Type> class TSharedPtr;

namespace SharedPtrInternal
{
    ///
    /// @struct FReferenceController
    /// @brief Thread-Safe reference controller for shared pointers.
    /// @details Manages the shared reference count and the lifetime of the managed object.
    ///
    struct FReferenceController
    {
        TAtomic<UInt64> SharedCount{ 1 };

        /// @brief Virtual destructor for derived classes.
        virtual ~FReferenceController () noexcept = default;

        /// @brief Destroys the managed object.
        virtual void DestroyObject () noexcept = 0;

        /// @brief Frees the memory block containing the controller.
        virtual void Deallocate () noexcept = 0;
    };

    ///
    /// @class TIntrusiveReferenceController
    /// @brief Stores the object and the reference count in a single memory block.
    /// @tparam Type The type of the managed object.
    ///
    template <typename Type> class TIntrusiveReferenceController final : public FReferenceController
    {
    public:

        /// @brief Properly aligned storage for the managed object
        alignas( Type ) Byte Storage[sizeof( Type )];

        /// @brief Constructs the managed object in place with forwarded arguments.
        template <typename... Arguments> explicit TIntrusiveReferenceController( Arguments &&...InArgs ) noexcept;

        /// @brief  Destroys the managed object.
        void DestroyObject () noexcept override;

        /// @brief Frees the memory block containing the controller.
        void Deallocate () noexcept override;

        /// @brief Returns a pointer to the managed object.
        [[nodiscard]] Type *GetObjectPtr () noexcept;
    };

    ///
    /// @classs TDefaultReferenceController
    /// @brief Controller for objects already allocated on the heap.
    /// @tparam Type The type of the managed object.
    ///
    template <typename Type> class TDefaultReferenceController final : public FReferenceController
    {
    public:

        /// @brief Constructs the controller with an existing pointer
        explicit TDefaultReferenceController ( Type *InObject ) noexcept;

        /// @brief Destroys the managed object via delete.
        void DestroyObject () noexcept override;

        /// @brief Frees the memory block containing the controller
        void Deallocate () noexcept override;

    private:

        Type *Object;
    };

    ///
    /// @struct TRawPtrProxy
    /// @brief Internal proxy used to allow implicit conversion from MakeShareable to Ref or Ptr.
    /// @tparam Type The type of the managed object.
    ///
    template <typename Type> struct TRawPtrProxy
    {
        /// @brief Constructs the proxy with an existing pointer
        explicit TRawPtrProxy ( Type *InObject ) noexcept;

        Type *Object;
    };

    /// @brief Creates a new TIntrusiveReferenceController instance with the given arguments.
    /// @tparam Type The type of the managed object.
    /// @param InArgs Arguments to forward to the constructor of ObjectType.
    /// @return A pointer to the newly created TIntrusiveReferenceController instance.
    template <typename Type, typename... Arguments>
    SharedPtrInternal::TIntrusiveReferenceController<Type> *NewIntrusiveReferenceController ( Arguments &&...InArgs ) noexcept;

    /// @brief Creates a new TDefaultReferenceController instance for the given object.
    /// @tparam Type The type of the managed object.
    /// @param InObject Pointer to the managed object.
    /// @return A pointer to othe newly created TDefaultReferenceController.
    template <typename Type>
    SharedPtrInternal::TDefaultReferenceController<Type> *NewDefaultReferenceController ( Type *InObject ) noexcept;
} // namespace SharedPtrInternal

///
/// @class TSharedRef
/// @brief A non-nullable shared reference smart pointer.
/// @tparam Type The type of the managed object.
///
template <typename Type> class TSharedRef final
{
    static_assert( not std::is_void_v<Type>, "TSharedRef<void> is not supported. Use a base class pointer." );

public:

    /// @brief Copy Constructor
    TSharedRef ( const TSharedRef &InOther ) noexcept;

    /// @brief Move Constructor
    TSharedRef ( TSharedRef &&InOther ) noexcept;

    /// @brief Proxy Constructor: Allows implicit conversion from MakeShareable to TSharedRef
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedRef( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy ) noexcept;

    /// @brief Upcast Constructor: Allows conversion from derived to base types
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedRef( const TSharedRef<OtherType> &InOther ) noexcept;

    /// @brief Destructor
    ~TSharedRef () noexcept;

public:

    /// @brief Assignment Operators
    TSharedRef &operator=( const TSharedRef &InOther ) noexcept;
    TSharedRef &operator=( TSharedRef &&InOther ) noexcept;

public:

    /// @brief Accessors Operators
    Type &operator*() const noexcept;
    Type *operator->() const noexcept;
    [[nodiscard]] Type *Get () const noexcept;

    /// @brief Returns the current number of shared references.
    [[nodiscard]] UInt64 GetSharedReferenceCount () const noexcept;

public:

    /// @brief Comparison Operators
    template <typename OtherType> Bool operator==( const TSharedRef<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator!=( const TSharedRef<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator==( const TSharedPtr<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator!=( const TSharedPtr<OtherType> &Other ) const noexcept;

private:

    /// @brief Private Constructor called by MakeSharedRef
    TSharedRef ( Type *InObject, SharedPtrInternal::FReferenceController *InController ) noexcept;

    /// @brief Releases the current reference.
    void Release () noexcept;

private:

    Type *Object;
    SharedPtrInternal::FReferenceController *Controller;

private:

    /// @brief Friend class
    template <typename ObjectType> friend class TSharedRef;
    template <typename ObjectType> friend class TSharedPtr;

    /// @brief Friend helpers
    template <typename ObjectType>
    friend TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject,
                                                  SharedPtrInternal::FReferenceController *InController ) noexcept;
    template <typename CastToType, typename CastFromType>
    friend TSharedRef<CastToType> StaticCastSharedRef ( const TSharedRef<CastFromType> &InSharedRef ) noexcept;
};

///
/// @class TSharedPtr
/// @brief A nullable shared pointer
/// @tparam Type The type of the managed object.
///
template <typename Type> class TSharedPtr final
{
public:

    /// @brief Default Constructor */
    TSharedPtr () noexcept;

    /// @brief NullPtrType Constructor
    TSharedPtr ( NullPtrType ) noexcept;

    /// @brief Constructor from TSharedRef
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedPtr( const TSharedRef<OtherType> &Other ) noexcept;

    /// @brief Copy Constructor
    TSharedPtr ( const TSharedPtr &Other ) noexcept;

    /// @brief Move Constructor
    TSharedPtr ( TSharedPtr &&Other ) noexcept;

    /// @brief Proxy Constructor: Allows implicit conversion from MakeShareable to TSharedPtr
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TSharedPtr( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy ) noexcept;

    /// @brief Destructor
    ~TSharedPtr () noexcept;

public:

    /// @brief Assignment Operators
    TSharedPtr &operator=( const TSharedPtr &Other ) noexcept;
    TSharedPtr &operator=( const TSharedRef<Type> &Other ) noexcept;

    /// @brief Accessors
    Type &operator*() const noexcept;
    Type *operator->() const noexcept;
    Type *Get () const noexcept;
    Bool IsValid () const noexcept;
    explicit operator Bool () const noexcept;

    /// @brief Resets the pointer to null.
    void Reset () noexcept;

    /// @brief Releases the current reference
    void Release () noexcept;

public:

    /// @brief Comparison Operators
    template <typename OtherType> Bool operator==( const TSharedPtr<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator!=( const TSharedPtr<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator==( const TSharedRef<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator!=( const TSharedRef<OtherType> &Other ) const noexcept;
    template <typename OtherType> Bool operator==( const OtherType *Other ) const noexcept;
    template <typename OtherType> Bool operator!=( const OtherType *Other ) const noexcept;

private:

    /// @brief Friend
    template <typename ObjectType> friend class TSharedPtr;
    template <typename CastToType, typename CastFromType>
    friend class TSharedPtr<CastToType> StaticCastSharedPtr ( const TSharedPtr<CastFromType> &InSharedPtr ) noexcept;

private:

    Type *Object;
    SharedPtrInternal::FReferenceController *Controller;
};

/// @brief Creates a TSharedRef from existing object and reference controller.
/// @param InObject Pointer to the managed object.
/// @param InController Pointer to the reference controller managing the object.
/// @return A TSharedRef managing the given object and reference controller.
template <typename ObjectType>
TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController ) noexcept;

/// @brief Creates a new TSharedRef instance with the given arguments.
/// @param InArgs Arguments to forward to the constructor of ObjectType.
/// @return A TSharedRef managing the newly created object.
template <typename ObjectType, typename... Arguments> TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs ) noexcept;

/// @brief Creates a TRawPtrProxy instance from the given object pointer.
/// @param InObject Pointer to the object to create a proxy for.
/// @return A TRawPtrProxy managing the given object.
template <typename ObjectType> SharedPtrInternal::TRawPtrProxy<ObjectType> MakeShareable ( ObjectType *InObject ) noexcept;
} // namespace Lumen

#include "Inline/SharedPtr.inl"
#include "Inline/SharedPtrInternal.inl"
#include "Inline/SharedRef.inl"
