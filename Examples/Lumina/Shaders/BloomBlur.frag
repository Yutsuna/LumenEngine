#version 460 core

layout(binding = 0) uniform sampler2D uSourceTex;
layout(std140, binding = 0) uniform UniformBlock0
{
    vec4 uDirection;
};

out vec4 fragColor;

const float WEIGHTS[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 texSize = vec2(textureSize(uSourceTex, 0));
    vec2 uv = gl_FragCoord.xy / texSize;
    vec2 stepUv = uDirection.xy / texSize;

    vec3 result = texture(uSourceTex, uv).rgb * WEIGHTS[0];
    for (int i = 1; i < 5; i++)
    {
        vec2 off = stepUv * (float(i) * 1.45);
        result += texture(uSourceTex, uv + off).rgb * WEIGHTS[i];
        result += texture(uSourceTex, uv - off).rgb * WEIGHTS[i];
    }
    fragColor = vec4(result, 1.0);
}
