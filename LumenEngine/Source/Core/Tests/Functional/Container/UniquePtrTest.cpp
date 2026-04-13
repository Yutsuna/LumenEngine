/**
 * @file UniquePtrTest.cpp
 * @brief Unit tests for TUniquePtr in LumenEngine
 */

#include "Container/UniquePtr.hpp"
#include <gtest/gtest.h>

class FTestObject
{
public:

    FTestObject ( LumenEngine::Int32 InValue = 0 ) : Value( InValue )
    {
        ++InstanceCount;
    }
    ~FTestObject ()
    {
        --InstanceCount;
    }

    LumenEngine::Int32 Value;
    static LumenEngine::Int32 InstanceCount;
};

LumenEngine::Int32 FTestObject::InstanceCount = 0;

class FDerivedObject : public FTestObject
{
public:

    FDerivedObject ( LumenEngine::Int32 InValue = 0 ) : FTestObject( InValue )
    {
        ++DerivedInstanceCount;
    }
    ~FDerivedObject ()
    {
        --DerivedInstanceCount;
    }

    static LumenEngine::Int32 DerivedInstanceCount;
};

LumenEngine::Int32 FDerivedObject::DerivedInstanceCount = 0;

struct FCustomDeleter
{
    void operator()( FTestObject *InPtr ) const
    {
        delete InPtr;
        ++DeleterCalledCount;
    }
    static LumenEngine::Int32 DeleterCalledCount;
};

LumenEngine::Int32 FCustomDeleter::DeleterCalledCount = 0;

class FUniquePtrTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        FTestObject::InstanceCount           = 0;
        FDerivedObject::DerivedInstanceCount = 0;
        FCustomDeleter::DeleterCalledCount   = 0;
    }
};

TEST_F( FUniquePtrTest, DefaultConstructor )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr;
    EXPECT_FALSE( Ptr.IsValid() );
    EXPECT_EQ( Ptr.Get(), nullptr );
}

TEST_F( FUniquePtrTest, NullptrConstructor )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr( nullptr );
    EXPECT_FALSE( Ptr.IsValid() );
    EXPECT_EQ( Ptr.Get(), nullptr );
}

TEST_F( FUniquePtrTest, RawPtrConstructor )
{
    {
        LumenEngine::TUniquePtr<FTestObject> Ptr( new FTestObject( 42 ) );
        EXPECT_TRUE( Ptr.IsValid() );
        EXPECT_EQ( Ptr->Value, 42 );
        EXPECT_EQ( FTestObject::InstanceCount, 1 );
    }
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
}

TEST_F( FUniquePtrTest, CustomDeleter )
{
    {
        LumenEngine::TUniquePtr<FTestObject, FCustomDeleter> Ptr( new FTestObject( 42 ) );
        EXPECT_TRUE( Ptr.IsValid() );
        EXPECT_EQ( FCustomDeleter::DeleterCalledCount, 0 );
    }
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
    EXPECT_EQ( FCustomDeleter::DeleterCalledCount, 1 );
}

TEST_F( FUniquePtrTest, MoveConstructor )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr1( new FTestObject( 10 ) );
    LumenEngine::TUniquePtr<FTestObject> Ptr2( std::move( Ptr1 ) );

    EXPECT_FALSE( Ptr1.IsValid() );
    EXPECT_TRUE( Ptr2.IsValid() );
    EXPECT_EQ( Ptr2->Value, 10 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
}

TEST_F( FUniquePtrTest, ConvertingMoveConstructor )
{
    LumenEngine::TUniquePtr<FDerivedObject> DerivedPtr( new FDerivedObject( 20 ) );
    LumenEngine::TUniquePtr<FTestObject> BasePtr( std::move( DerivedPtr ) );

    EXPECT_FALSE( DerivedPtr.IsValid() );
    EXPECT_TRUE( BasePtr.IsValid() );
    EXPECT_EQ( BasePtr->Value, 20 );
    EXPECT_EQ( FDerivedObject::DerivedInstanceCount, 1 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
}

TEST_F( FUniquePtrTest, MoveAssignment )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr1( new FTestObject( 10 ) );
    LumenEngine::TUniquePtr<FTestObject> Ptr2( new FTestObject( 20 ) );

    Ptr2 = std::move( Ptr1 );

    EXPECT_FALSE( Ptr1.IsValid() );
    EXPECT_TRUE( Ptr2.IsValid() );
    EXPECT_EQ( Ptr2->Value, 10 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
}

TEST_F( FUniquePtrTest, ConvertingMoveAssignment )
{
    LumenEngine::TUniquePtr<FDerivedObject> DerivedPtr( new FDerivedObject( 20 ) );
    LumenEngine::TUniquePtr<FTestObject> BasePtr( new FTestObject( 10 ) );

    BasePtr = std::move( DerivedPtr );

    EXPECT_FALSE( DerivedPtr.IsValid() );
    EXPECT_TRUE( BasePtr.IsValid() );
    EXPECT_EQ( BasePtr->Value, 20 );
    EXPECT_EQ( FDerivedObject::DerivedInstanceCount, 1 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
}

TEST_F( FUniquePtrTest, NullptrAssignment )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr( new FTestObject( 42 ) );
    EXPECT_TRUE( Ptr.IsValid() );

    Ptr = nullptr;

    EXPECT_FALSE( Ptr.IsValid() );
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
}

TEST_F( FUniquePtrTest, Accessors )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr( new FTestObject( 42 ) );
    EXPECT_EQ( ( *Ptr ).Value, 42 );
    EXPECT_EQ( Ptr->Value, 42 );
    EXPECT_EQ( Ptr.Get()->Value, 42 );
}

TEST_F( FUniquePtrTest, Release )
{
    FTestObject *RawPtr = nullptr;
    {
        LumenEngine::TUniquePtr<FTestObject> Ptr( new FTestObject( 42 ) );
        RawPtr = Ptr.Release();
        EXPECT_FALSE( Ptr.IsValid() );
        EXPECT_EQ( FTestObject::InstanceCount, 1 );
    }
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
    delete RawPtr;
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
}

TEST_F( FUniquePtrTest, Reset )
{
    LumenEngine::TUniquePtr<FTestObject> Ptr( new FTestObject( 42 ) );
    Ptr.Reset( new FTestObject( 24 ) );

    EXPECT_EQ( Ptr->Value, 24 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );

    Ptr.Reset();
    EXPECT_FALSE( Ptr.IsValid() );
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
}

TEST_F( FUniquePtrTest, MakeUnique )
{
    auto Ptr = LumenEngine::MakeUnique<FTestObject>( 42 );
    EXPECT_TRUE( Ptr.IsValid() );
    EXPECT_EQ( Ptr->Value, 42 );
    EXPECT_EQ( FTestObject::InstanceCount, 1 );
}

TEST_F( FUniquePtrTest, ArraySpecialization )
{
    {
        LumenEngine::TUniquePtr<FTestObject[]> Ptr( new FTestObject[3] );
        EXPECT_TRUE( Ptr.IsValid() );
        EXPECT_EQ( FTestObject::InstanceCount, 3 );

        Ptr[0].Value = 1;
        Ptr[1].Value = 2;
        Ptr[2].Value = 3;

        EXPECT_EQ( Ptr[0].Value, 1 );
        EXPECT_EQ( Ptr[1].Value, 2 );
        EXPECT_EQ( Ptr[2].Value, 3 );
    }
    EXPECT_EQ( FTestObject::InstanceCount, 0 );
}

TEST_F( FUniquePtrTest, MakeUniqueArray )
{
    auto Ptr = LumenEngine::MakeUnique<FTestObject[]>( 5 );
    EXPECT_TRUE( Ptr.IsValid() );
    EXPECT_EQ( FTestObject::InstanceCount, 5 );
}
