#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in ivec4 joints;
layout(location = 6) in vec4 weights;
layout(location = 0) out vec3 fragColor;



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

float outline_thickness = 0.1;
vec3 outline_color = vec3(1.0, 1.0, 0.3);
void main(){
    vec4 positionWorld = vec4(position - normal * outline_thickness , 1.0);
    positionWorld = push.modelMatrix * positionWorld;
    gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionWorld);

    fragColor = outline_color;
}