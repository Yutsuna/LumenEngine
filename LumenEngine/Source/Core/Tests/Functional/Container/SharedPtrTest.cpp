/**
 * @file SharedPtrTest.cpp
 * @brief Unit tests for TSharedPtr and TSharedRef in LumenEngine
 */

#include "Container/SharedPtr.hpp"
#include <gtest/gtest.h>

class FSharedTestObject
{
public:

    FSharedTestObject ( LumenEngine::Int32 InValue = 0 ) : Value( InValue )
    {
        ++InstanceCount;
    }
    virtual ~FSharedTestObject ()
    {
        --InstanceCount;
    }

    LumenEngine::Int32 Value;
    static LumenEngine::Int32 InstanceCount;
};

LumenEngine::Int32 FSharedTestObject::InstanceCount = 0;

class FSharedDerivedObject : public FSharedTestObject
{
public:

    FSharedDerivedObject ( LumenEngine::Int32 InValue = 0 ) : FSharedTestObject( InValue )
    {
        ++DerivedInstanceCount;
    }
    ~FSharedDerivedObject () override
    {
        --DerivedInstanceCount;
    }

    static LumenEngine::Int32 DerivedInstanceCount;
};

LumenEngine::Int32 FSharedDerivedObject::DerivedInstanceCount = 0;

class FSharedPtrTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        FSharedTestObject::InstanceCount           = 0;
        FSharedDerivedObject::DerivedInstanceCount = 0;
    }
};

TEST_F( FSharedPtrTest, SharedRefConstructor )
{
    {
        LumenEngine::TSharedRef<FSharedTestObject> Ref = LumenEngine::MakeShared<FSharedTestObject>( 42 );
        EXPECT_EQ( Ref->Value, 42 );
        EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
        EXPECT_EQ( Ref.GetSharedReferenceCount(), 1 );
    }
    EXPECT_EQ( FSharedTestObject::InstanceCount, 0 );
}

TEST_F( FSharedPtrTest, SharedRefCopyConstructor )
{
    LumenEngine::TSharedRef<FSharedTestObject> Ref1 = LumenEngine::MakeShared<FSharedTestObject>( 42 );
    {
        LumenEngine::TSharedRef<FSharedTestObject> Ref2( Ref1 );
        EXPECT_EQ( Ref1.GetSharedReferenceCount(), 2 );
        EXPECT_EQ( Ref2.GetSharedReferenceCount(), 2 );
    }
    EXPECT_EQ( Ref1.GetSharedReferenceCount(), 1 );
}

TEST_F( FSharedPtrTest, SharedRefUpcast )
{
    LumenEngine::TSharedRef<FSharedDerivedObject> DerivedRef = LumenEngine::MakeShared<FSharedDerivedObject>( 42 );
    LumenEngine::TSharedRef<FSharedTestObject> BaseRef( DerivedRef );

    EXPECT_EQ( BaseRef.GetSharedReferenceCount(), 2 );
    EXPECT_EQ( FSharedDerivedObject::DerivedInstanceCount, 1 );
}

TEST_F( FSharedPtrTest, SharedPtrDefaultConstructor )
{
    LumenEngine::TSharedPtr<FSharedTestObject> Ptr;
    EXPECT_FALSE( Ptr.IsValid() );
}

TEST_F( FSharedPtrTest, SharedPtrFromRef )
{
    LumenEngine::TSharedRef<FSharedTestObject> Ref = LumenEngine::MakeShared<FSharedTestObject>( 42 );
    LumenEngine::TSharedPtr<FSharedTestObject> Ptr( Ref );

    EXPECT_TRUE( Ptr.IsValid() );
    EXPECT_EQ( Ref.GetSharedReferenceCount(), 2 );
}

TEST_F( FSharedPtrTest, SharedPtrCopyAssignment )
{
    LumenEngine::TSharedPtr<FSharedTestObject> Ptr1 = LumenEngine::MakeShared<FSharedTestObject>( 10 );
    LumenEngine::TSharedPtr<FSharedTestObject> Ptr2;

    Ptr2 = Ptr1;
    EXPECT_TRUE( Ptr2.IsValid() );
    EXPECT_EQ( Ptr1.Get(), Ptr2.Get() );
    EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
}

TEST_F( FSharedPtrTest, SharedPtrReset )
{
    LumenEngine::TSharedPtr<FSharedTestObject> Ptr = LumenEngine::MakeShared<FSharedTestObject>( 42 );
    EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
    Ptr.Reset();
    EXPECT_FALSE( Ptr.IsValid() );
    EXPECT_EQ( FSharedTestObject::InstanceCount, 0 );
}

TEST_F( FSharedPtrTest, MakeShareable )
{
    {
        LumenEngine::TSharedPtr<FSharedTestObject> Ptr = LumenEngine::MakeShareable( new FSharedTestObject( 42 ) );
        EXPECT_TRUE( Ptr.IsValid() );
        EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
    }
    EXPECT_EQ( FSharedTestObject::InstanceCount, 0 );
}

TEST_F( FSharedPtrTest, StaticCastSharedRef )
{
    LumenEngine::TSharedRef<FSharedDerivedObject> DerivedRef = LumenEngine::MakeShared<FSharedDerivedObject>( 42 );
    LumenEngine::TSharedRef<FSharedTestObject> BaseRef       = LumenEngine::StaticCastSharedRef<FSharedTestObject>( DerivedRef );

    EXPECT_EQ( BaseRef.GetSharedReferenceCount(), 2 );

    LumenEngine::TSharedRef<FSharedDerivedObject> DerivedRef2 = LumenEngine::StaticCastSharedRef<FSharedDerivedObject>( BaseRef );
    EXPECT_EQ( BaseRef.GetSharedReferenceCount(), 3 );
}

TEST_F( FSharedPtrTest, StaticCastSharedPtr )
{
    LumenEngine::TSharedPtr<FSharedDerivedObject> DerivedPtr = LumenEngine::MakeShared<FSharedDerivedObject>( 42 );
    LumenEngine::TSharedPtr<FSharedTestObject> BasePtr       = LumenEngine::StaticCastSharedPtr<FSharedTestObject>( DerivedPtr );

    EXPECT_TRUE( BasePtr.IsValid() );
}

TEST_F( FSharedPtrTest, ComplexReferenceCounting )
{
    LumenEngine::TSharedPtr<FSharedTestObject> OuterPtr;
    {
        LumenEngine::TSharedRef<FSharedTestObject> Ref = LumenEngine::MakeShared<FSharedTestObject>( 1 );
        OuterPtr                                       = Ref;
        {
            LumenEngine::TSharedPtr<FSharedTestObject> InnerPtr = OuterPtr;
            EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
        }
        EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
    }
    EXPECT_EQ( FSharedTestObject::InstanceCount, 1 );
    OuterPtr.Reset();
    EXPECT_EQ( FSharedTestObject::InstanceCount, 0 );
}
