/**
 * @file SpirvReflector.cpp
 * @brief Implementation of SPIR-V reflection using SPIRV-Cross.
 */

#include "Shader/Internal/SpirvReflector.hpp"

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace LumenEngine
{

namespace Internal
{

    namespace
    {

        /** @brief Internal helper to extract bindings from SPIR-V resources. */
        void ExtractBindings ( const spirv_cross::CompilerGLSL &InCompiler,
                               const spirv_cross::SmallVector<spirv_cross::Resource> &InResList,
                               const FString &InTypeName,
                               const EShaderStage::Type InStage,
                               FShaderReflection &OutReflection )
        {
            for ( const spirv_cross::Resource &Res : InResList )
            {
                FShaderResourceBinding Binding;
                Binding.Name           = Res.name;
                Binding.Set            = InCompiler.get_decoration( Res.id, spv::DecorationDescriptorSet );
                Binding.Binding        = InCompiler.get_decoration( Res.id, spv::DecorationBinding );
                Binding.DexcriptorType = InTypeName;

                const spirv_cross::SPIRType &Type = InCompiler.get_type( Res.type_id );
                Binding.ArraySize                 = Type.array.empty() ? 1U : Type.array[0];
                Binding.StageMask                 = static_cast<UInt8>( 1U << InStage );

                OutReflection.ResourceBindings.push_back( std::move( Binding ) );
            }
        }

    } // namespace

} // namespace Internal

} // namespace LumenEngine

LumenEngine::Bool LumenEngine::Internal::FSpirvReflector::Reflect ( const FSpirVBlob &InSpirV,
                                                                    const EShaderStage::Type InStage,
                                                                    FShaderReflection &OutReflection,
                                                                    FString &OutError ) noexcept
{
    if ( InSpirV.empty() )
    {
        OutError = "SPIR-V blob is empty.";
        return false;
    }

    try
    {
        /**
         * INFO: We use CompilerGLSL directly here.
         * This avoids virtual table lookup issues with the base class across shared library boundaries.
         */
        spirv_cross::CompilerGLSL Compiler( InSpirV.data(), InSpirV.size() );
        const spirv_cross::ShaderResources Resources = Compiler.get_shader_resources();

        ExtractBindings( Compiler, Resources.uniform_buffers, "UniformBuffer", InStage, OutReflection );
        ExtractBindings( Compiler, Resources.sampled_images, "SampledImage", InStage, OutReflection );
        ExtractBindings( Compiler, Resources.storage_buffers, "StorageBuffer", InStage, OutReflection );
        ExtractBindings( Compiler, Resources.storage_images, "StorageImage", InStage, OutReflection );

        if ( InStage == EShaderStage::Vertex )
        {
            for ( const spirv_cross::Resource &Res : Resources.stage_inputs )
            {
                FShaderVertexInput Input;
                Input.Name     = Res.name;
                Input.Location = Compiler.get_decoration( Res.id, spv::DecorationLocation );

                const spirv_cross::SPIRType &Type = Compiler.get_type( Res.type_id );

                switch ( Type.basetype )
                {
                case spirv_cross::SPIRType::Float:
                    Input.TypeName = "Float";
                    break;
                case spirv_cross::SPIRType::Int:
                    Input.TypeName = "Int";
                    break;
                case spirv_cross::SPIRType::UInt:
                    Input.TypeName = "UInt";
                    break;
                default:
                    Input.TypeName = "Unknown";
                    break;
                }

                OutReflection.VertexInputs.push_back( std::move( Input ) );
            }
        }

        for ( const spirv_cross::Resource &Res : Resources.push_constant_buffers )
        {
            FShaderPushConstantRange Range;
            Range.Name                        = Res.name;
            Range.Offset                      = 0U;
            const spirv_cross::SPIRType &Type = Compiler.get_type( Res.base_type_id );
            Range.Size                        = static_cast<UInt32>( Compiler.get_declared_struct_size( Type ) );

            OutReflection.PushConstantRanges.push_back( std::move( Range ) );
        }

        if ( InStage == EShaderStage::Compute )
        {
            OutReflection.LocalSizeX = Compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 0U );
            OutReflection.LocalSizeY = Compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 1U );
            OutReflection.LocalSizeZ = Compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 2U );
        }

        return true;
    }
    catch ( const std::exception &Exception )
    {
        OutError = Exception.what();
        return false;
    }
}