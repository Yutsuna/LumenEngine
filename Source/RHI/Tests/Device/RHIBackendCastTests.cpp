/**
 * @file RHIBackendCastTests.cpp
 * @brief Tests tag-checked backend casts.
 */

#include <Device/NullDevice.hpp>
#include <Device/RHIBackendCast.hpp>
#include <Templates/Optional.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/NullObject.hpp>

namespace Lumen
{

LUMEN_TEST( RHIBackendCast, AcceptsMatchingTagAndRejectsOtherTypes )
{
    TSharedRef<IRHIObject> Object = MakeShared<Tests::FNullObject>();
    LUMEN_CHECK_TRUE( TryRHIBackendCast<Tests::FNullObject>( Object ).HasValue() );
    LUMEN_CHECK_FALSE( TryRHIBackendCast<Tests::FNullDevice>( Object ).HasValue() );
    const TOptional<Tests::FNullObject *> Matching = TryRHIBackendCast<Tests::FNullObject>( *Object );
    LUMEN_CHECK_HAS_VALUE( Matching );
    LUMEN_CHECK_EQ( *Matching, static_cast<Tests::FNullObject *>( Object.Get() ) );
    LUMEN_CHECK_FALSE( TryRHIBackendCast<Tests::FNullDevice>( *Object ).HasValue() );
}

} // namespace Lumen
