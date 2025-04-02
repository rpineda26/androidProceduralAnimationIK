#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;
struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};
layout(push_constant) uniform PushConstants {
    vec3 startPos;
    float pad1;
    vec3 endPos;
    float pad2;
    vec4 color;
} push;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    PointLight lights[10];
    int lightCount;
    int selectedLight;
    float time;
    
} ubo;
void main() {

    outColor =fragColor;
}
