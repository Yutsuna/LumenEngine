/**
 * @file NullObject.hpp
 * @brief Declares a minimal concrete RHI object for contract tests.
 */

#pragma once

#include <RHI/Types/RHIObjectBase.hpp>
#include <Templates/Hash.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullObject
         * @brief Deterministic object with a configurable identifier.
         */
        class FNullObject : public FRHIObjectBase
        {
        public:

            /** @brief Creates a Null object with the requested identifier. */
            explicit FNullObject ( const FRHIObjectId InId = FRHIObjectId{ 1ULL } ) noexcept
                : FRHIObjectBase( InId, FHash::TypeHash<FNullObject>() )
            {
                /* */
            }
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
