/**
 * @file BasePassFeature.cpp
 * @brief Implementation of the base pass rendering feature.
 */

#include "Graphics/Features/BasePassFeature.hpp"
#include "RHI/RHI.hpp"

namespace
{

inline void ExecuteDrawCommand ( LumenEngine::RHI::IRHICommandList &InCmdList, const LumenEngine::Renderer::FDrawCommand &InCommand )
{
    if ( InCommand.Mesh.IsValid() and InCommand.Shader.IsValid() )
    {
        InCmdList.BindPipeline( InCommand.Shader );
        InCmdList.PushConstants( InCommand.Shader, &InCommand.Transform, sizeof( LumenEngine::Maths::FMatrix4x4f ), 0 );
        InCmdList.DrawMesh( InCommand.Mesh );
    }
}

} // namespace

void LumenEngine::Renderer::FBasePassFeature::Initialize ( RHI::IRHI *InRHI )
{
    RHI = InRHI;
}

void LumenEngine::Renderer::FBasePassFeature::Execute ( RHI::IRHICommandList &InCmdList, const FRenderPacket &InPacket, const RHI::FGlobalUniformData & /*InUniforms*/ )
{
    if ( RHI == nullptr )
    {
        return;
    }

    if ( not InPacket.SceneSnapshot.Transforms.empty() )
    {
        InCmdList.DrawScene( InPacket.SceneSnapshot, InPacket.ClearColor );
        return;
    }

    InCmdList.BeginRendering( InPacket.ClearColor );

    for ( const FDrawCommand &Command : InPacket.DrawCommands )
    {
        ExecuteDrawCommand( InCmdList, Command );
    }

    InCmdList.EndRendering();
}
