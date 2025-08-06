#version 450

struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};

layout(push_constant) uniform PushConstant {
    mat4 modelMatrix;
    float size;
    vec4 color;
} push;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int pointLightCount;
    int selectedLight;
    float time;
} ubo;


void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * push.modelMatrix * vec4(0.0, 0.0, 0.0, 1.0);
    float adjustedSize = push.size * 30.0;
    if (push.color.w > 1.0) {
        // Landscape mode - make points narrower
        adjustedSize *= (1.0 / push.color.w);
    }
    gl_PointSize = adjustedSize;
}