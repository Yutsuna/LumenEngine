/**
 * @file CoreTypesTest.cpp
 * @brief Unit tests for core type definitions in LumenEngine
 */

#include "CoreTypes.hpp"

#include <gtest/gtest.h>

TEST( Core, TypesSize )
{
    EXPECT_EQ( sizeof( LumenEngine::Int8 ), 1 );
    EXPECT_EQ( sizeof( LumenEngine::Int16 ), 2 );
    EXPECT_EQ( sizeof( LumenEngine::Int32 ), 4 );
    EXPECT_EQ( sizeof( LumenEngine::Int64 ), 8 );
    EXPECT_EQ( sizeof( LumenEngine::UInt8 ), 1 );
    EXPECT_EQ( sizeof( LumenEngine::UInt16 ), 2 );
    EXPECT_EQ( sizeof( LumenEngine::UInt32 ), 4 );
    EXPECT_EQ( sizeof( LumenEngine::UInt64 ), 8 );
    EXPECT_EQ( sizeof( LumenEngine::Float32 ), 4 );
    EXPECT_EQ( sizeof( LumenEngine::Float64 ), 8 );
    EXPECT_EQ( sizeof( LumenEngine::AnsiChar ), 1 );
    EXPECT_EQ( sizeof( LumenEngine::WideChar ), 4 );
    EXPECT_EQ( sizeof( LumenEngine::Bool ), 1 );
    EXPECT_EQ( sizeof( LumenEngine::USize ), sizeof( std::size_t ) );
    EXPECT_EQ( sizeof( LumenEngine::ISize ), sizeof( std::ptrdiff_t ) );
}
