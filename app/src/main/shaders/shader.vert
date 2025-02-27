#version 450

// Input vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
// Colour passed to the fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

// Uniform buffer containing an MVP matrix.
layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
} ubo;

void main() {
    gl_Position = ubo.MVP * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}