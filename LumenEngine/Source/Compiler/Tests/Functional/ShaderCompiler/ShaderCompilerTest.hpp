/**
 * @file ShaderCompilerTest.hpp
 * @brief Common utilities and constants for ShaderCompiler tests.
 */

#pragma once

#include "ShaderCompiler/ShaderCompilerTypes.hpp"

#include <gtest/gtest.h>

namespace LumenEngine
{

namespace Compiler
{

    inline constexpr const AnsiChar *GMinimalVertexShader = R"glsl(
#version 450
layout(location = 0) in vec3 inPosition;
void main() {
    gl_Position = vec4(inPosition, 1.0);
}
)glsl";

    inline constexpr const AnsiChar *GMinimalFragmentShader = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)glsl";

    inline constexpr const AnsiChar *GComputeShader = R"glsl(
#version 450
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(set = 0, binding = 0) buffer Storage {
    float data[];
} b;
void main() {
    b.data[gl_GlobalInvocationID.x] *= 2.0;
}
)glsl";

    inline constexpr const AnsiChar *GShaderWithUniform = R"glsl(
#version 450
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
} ubo;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1.0);
}
)glsl";

    inline constexpr const AnsiChar *GShaderWithMacro = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main() {
#ifdef TEST_MACRO
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
#else
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
#endif
}
)glsl";

} // namespace Compiler

} // namespace LumenEngine
