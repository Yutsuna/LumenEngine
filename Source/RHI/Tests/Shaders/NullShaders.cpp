/**
 * @file NullShaders.cpp
 * @brief Implements owned Null shaders.
 */

#include <Shaders/NullShaders.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullShader::FNullShader ( const FRHIShaderDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullShader>() ), Desc( InDesc ),
      OwnedBytecode( InDesc.Bytecode.begin(), InDesc.Bytecode.end() )
{
    Desc.Bytecode = TSpan<const Byte>( OwnedBytecode.data(), OwnedBytecode.size() );
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullShader )
const Lumen::RHI::FRHIShaderDesc &Lumen::RHI::Tests::FNullShader::GetDesc () const noexcept
{
    return Desc;
}
