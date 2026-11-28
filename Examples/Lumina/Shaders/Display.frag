#version 460 core

layout(binding = 0) uniform sampler2D uAccumTex;
layout(binding = 1) uniform sampler2D uBloomTex;
layout(std140, binding = 0) uniform UniformBlock0
{
    vec4 uSampleOutput;
};

out vec4 fragColor;

const float EXPOSURE = 1.15;
const float INV_GAMMA = 1.0 / 2.2;
const float BLOOM_INTENSITY = 0.42;

vec3 acesFilmic(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    ivec2 texSize = textureSize(uAccumTex, 0);
    vec2 uv = gl_FragCoord.xy / uSampleOutput.yz;
    ivec2 pix = clamp(ivec2(uv * vec2(texSize)), ivec2(0), texSize - ivec2(1));
    float invSamples = 1.0 / float(max(uint(uSampleOutput.x), 1u));

    vec3 hdrCenter = texelFetch(uAccumTex, pix, 0).rgb * invSamples * EXPOSURE;
    vec3 bloom = texture(uBloomTex, uv).rgb * BLOOM_INTENSITY;

    vec3 mapped = acesFilmic(hdrCenter + bloom);
    fragColor = vec4(pow(mapped, vec3(INV_GAMMA)), 1.0);
}
