#version 450

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
    PointLight pointLights[10];
    int pointLightCount;
    int selectedLight;
    float time;
} ubo;

layout(location = 0 ) out vec4 fragColor;

void main() {
    vec3 position = (gl_VertexIndex == 0) ? push.startPos : push.endPos;

    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(position, 1.0);

    fragColor = push.color;
}