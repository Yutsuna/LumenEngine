/// @file UnitTestAssertionFailure.cpp
/// @brief Unit test assertion failure implementation.

#include <Tests/UnitTestAssertionFailure.hpp>

Lumen::Tests::FUnitTestAssertionFailure::FUnitTestAssertionFailure ( FString InMessage,
                                                                     const std::source_location &InLocation ) noexcept
    : Message( std::move( InMessage ) ), Location( InLocation )
{
    /// Empty
}

[[nodiscard]] const Lumen::AnsiChar *Lumen::Tests::FUnitTestAssertionFailure::what () const noexcept
{
    return Message.c_str();
}
