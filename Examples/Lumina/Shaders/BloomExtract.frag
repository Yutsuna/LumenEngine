#version 460 core

layout(binding = 0) uniform sampler2D uAccumTex;
layout(std140, binding = 0) uniform UniformBlock0
{
    vec4 uSampleCount;
};

out vec4 fragColor;

const float EXPOSURE = 1.15;
const float BLOOM_THRESHOLD = 1.35;

vec3 extractBloom(vec3 hdr)
{
    float lum = max(hdr.r, max(hdr.g, hdr.b));
    float soft = smoothstep(BLOOM_THRESHOLD * 0.65, BLOOM_THRESHOLD * 1.6, lum);
    vec3 excess = max(vec3(0.0), hdr - vec3(BLOOM_THRESHOLD)) * soft;
    return min(excess, vec3(9.0));
}

void main()
{
    ivec2 fullSize = textureSize(uAccumTex, 0);
    ivec2 bloomSize = max(ivec2(1), fullSize / 4);
    vec2 uv = gl_FragCoord.xy / vec2(bloomSize);
    ivec2 basePix = clamp(ivec2(uv * vec2(fullSize)), ivec2(0), fullSize - ivec2(1));
    float invSamples = 1.0 / float(max(uint(uSampleCount.x), 1u));

    vec3 sum = vec3(0.0);
    for (int dy = 0; dy <= 1; dy++)
    {
        for (int dx = 0; dx <= 1; dx++)
        {
            ivec2 p = clamp(basePix + ivec2(dx * 2 - 1, dy * 2 - 1), ivec2(0), fullSize - ivec2(1));
            vec3 hdr = texelFetch(uAccumTex, p, 0).rgb * invSamples * EXPOSURE;
            sum += extractBloom(hdr);
        }
    }
    fragColor = vec4(sum * 0.25, 1.0);
}
