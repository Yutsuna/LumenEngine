#version 450

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inUV;
layout( location = 3 ) in vec3 inTangent;

layout( set = 0, binding = 0 ) uniform GlobalData
{
    mat4 ViewProjectionMatrix;
    float Time;
    float DeltaTime;
}
ubo;

layout( location = 0 ) out vec3 fragColor;

void main ()
{
    vec3 pos = inPosition;

    pos.y += sin( ubo.Time * 3.0 + pos.x ) * 0.2;

    gl_Position = ubo.ViewProjectionMatrix * vec4( pos, 1.0 );
    fragColor   = inNormal;
}