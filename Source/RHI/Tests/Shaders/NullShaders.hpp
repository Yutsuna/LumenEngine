/**
 * @file NullShaders.hpp
 * @brief Declares the owned Null shader.
 */

#pragma once

#include <RHI/Shaders/IRHIShader.hpp>
#include <Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullShader
         * @brief Shader owning a stable copy of its bytecode.
         */
        class FNullShader final : public IRHIShader
        {
        public:

            FNullShader ( const FRHIShaderDesc &InDesc, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            const FRHIShaderDesc &GetDesc () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHIShaderDesc Desc;
            TVector<Byte> OwnedBytecode;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
