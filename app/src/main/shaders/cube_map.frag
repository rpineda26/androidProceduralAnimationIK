#version 450
layout(location = 0) in vec3 fragUVW;
layout(location = 0) out vec4 outColor;
struct PointLight {
    vec4 position;
    vec4 color;
    float radius;
    int objId;
};

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

layout(push_constant) uniform Push {
    mat4 modelMatrix;
} push;

layout(set = 1, binding = 0) uniform samplerCube cubeMap;
void main(){
    outColor = texture(cubeMap, fragUVW);
}