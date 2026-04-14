/**
 * @file TriangleActor.cpp
 * @brief Implementation of the ATriangleMeshActor class
 */

#include "Actors/Triangle.hpp"

namespace
{
inline void CreateTriangle ( LumenEngine::Renderer::FRenderMesh *OutMesh )
{
    OutMesh->Vertices = { { { 0.0F, -0.5F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                          { { 0.5F, 0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                          { { -0.5F, 0.5F, 0.0F }, { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } } };
    OutMesh->Indices  = { 0, 1, 2 };
}

inline void CreateTriangleShader ( LumenEngine::Renderer::FRenderShader *OutShader )
{
    OutShader->VertexPath   = "Shaders/Triangle.vert.spv";
    OutShader->FragmentPath = "Shaders/Triangle.frag.spv";
}
} // namespace

LumenEngine::ATriangleMeshActor::ATriangleMeshActor ( ActorID InId ) noexcept : AStaticMeshActor( InId )
{
    CreateTriangle( Mesh );
    CreateTriangleShader( Shader );
}