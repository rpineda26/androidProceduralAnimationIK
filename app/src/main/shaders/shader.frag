#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texCoords;
layout(binding = 1) uniform sampler2D samp;

// Output colour for the fragment
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(color,1.0);
}