#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};

layout(push_constant) uniform Push {
    mat4 modelMatrix;      // Transform to position/orient/scale cone
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

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * positionWorld;

    // Pass world position to fragment shader
    fragPosWorld = positionWorld.xyz;

    // Transform normal to world space
    // Use normal matrix (inverse transpose of model matrix)
    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    fragNormalWorld = normalize(normalMatrix*normal);
    // Pass color to fragment shader
    fragColor = push.color.rgb;
}