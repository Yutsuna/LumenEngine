/**
 * @file BasePassFeature.cpp
 * @brief Implementation of the base pass rendering feature.
 */

#include "Graphics/Features/BasePassFeature.hpp"
#include "RHI/RHI.hpp"

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

    InCmdList.BeginRendering( InPacket.ClearColor );

    for ( const FDrawCommand &Command : InPacket.DrawCommands )
    {
        ExecuteDrawCommand( InCmdList, Command );
    }

    InCmdList.EndRendering();
}

void LumenEngine::Renderer::FBasePassFeature::ExecuteDrawCommand ( RHI::IRHICommandList &InCmdList, const FDrawCommand &InCommand ) const
{
    if ( InCommand.Mesh == nullptr or InCommand.Shader == nullptr )
    {
        return;
    }

    if ( not InCommand.Mesh->RenderHandle.IsValid() )
    {
        InCommand.Mesh->RenderHandle = RHI->CreateMesh( InCommand.Mesh->Vertices, InCommand.Mesh->Indices );
    }

    if ( not InCommand.Shader->RenderHandle.IsValid() )
    {
        InCommand.Shader->RenderHandle = RHI->CreatePipeline( InCommand.Shader->VertexPath, InCommand.Shader->FragmentPath );
    }

    if ( InCommand.Mesh->RenderHandle.IsValid() && InCommand.Shader->RenderHandle.IsValid() )
    {
        InCmdList.BindPipeline( InCommand.Shader->RenderHandle );
        InCmdList.DrawMesh( InCommand.Mesh->RenderHandle );
    }
}