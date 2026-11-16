/**
 * @file RHIBackendCastTests.cpp
 * @brief Tests tag-checked backend casts.
 */

#include <Device/NullDevice.hpp>
#include <Device/RHIBackendCast.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/NullObject.hpp>

namespace Lumen
{

LUMEN_TEST( RHIBackendCast, AcceptsMatchingTagAndRejectsOtherTypes )
{
    TSharedRef<IRHIObject> Object = MakeShared<Tests::FNullObject>();
    LUMEN_CHECK_TRUE( TryRHIBackendCast<Tests::FNullObject>( Object ).HasValue() );
    LUMEN_CHECK_FALSE( TryRHIBackendCast<Tests::FNullDevice>( Object ).HasValue() );
    LUMEN_CHECK_NOT_EQ( TryRHIBackendCast<Tests::FNullObject>( *Object ), nullptr );
    LUMEN_CHECK_EQ( TryRHIBackendCast<Tests::FNullDevice>( *Object ), nullptr );
}

} // namespace Lumen
