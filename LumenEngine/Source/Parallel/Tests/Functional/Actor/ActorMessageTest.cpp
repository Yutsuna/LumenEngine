/**
 * @file ActorMessageTest.cpp
 * @brief Functional tests for FMessage.
 */

#include "Actor/ActorMessage.hpp"
#include <gtest/gtest.h>

namespace
{
struct FTestPayload
{
    LumenEngine::UInt64 ID;
    LumenEngine::Float32 Health;
};
} // namespace

TEST( ParallelActor, MessageIntegrity )
{
    using namespace LumenEngine;

    /** Test simple message */
    const FMessage SimpleMsg = FMessage::Make( 100U, 42ULL );
    EXPECT_EQ( SimpleMsg.Type, 100U );
    EXPECT_EQ( SimpleMsg.Sender, 42ULL );

    /** Test payload message */
    FTestPayload OriginalData{ .ID = 1337ULL, .Health = 95.5F };
    FMessage PayloadMsg = FMessage::Make<FTestPayload>( 1U, 1ULL, OriginalData );

    EXPECT_EQ( PayloadMsg.Type, 1U );

    /** Branch: GetPayload extraction */
    const FTestPayload &ExtractedData = PayloadMsg.GetPayload<FTestPayload>();
    EXPECT_EQ( ExtractedData.ID, 1337ULL );
    EXPECT_FLOAT_EQ( ExtractedData.Health, 95.5F );
}

TEST( ParallelActor, MessageStaticConstraints )
{
    using namespace LumenEngine;

    /** Ensure memory layout is preserved for cache performance */
    EXPECT_EQ( sizeof( FMessage ), 128ULL );
    EXPECT_EQ( alignof( FMessage ), 64ULL );
}