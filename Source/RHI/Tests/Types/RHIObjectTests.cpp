/**
 * @file RHIObjectTests.cpp
 * @brief Tests the common RHI object contract.
 */

#include "Types/NullObject.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIObject, ProvidesIdentityNameAndBackendTag )
    {
        Tests::FNullObject Object( FRHIObjectId{ 9ULL } );
        LUMEN_CHECK_EQ( Object.GetId().GetValue(), 9ULL );
        LUMEN_CHECK_NOT_EQ( Object.GetBackendTypeTag(), 0ULL );
        Object.SetDebugName( "TestObject" );
        LUMEN_CHECK_TRUE( Object.GetDebugName() == "TestObject" );
    }

} // namespace RHI

} // namespace Lumen
