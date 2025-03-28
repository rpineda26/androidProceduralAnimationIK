#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) out vec2 fragOffset;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float isSelected; // float to be used as a boolean
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


float EPSILON = 0.0001;
void main() {
    fragOffset = OFFSETS[gl_VertexIndex % 6];
    vec3 cameraRightWorld = {ubo.viewMatrix[0][0], ubo.viewMatrix[1][0], ubo.viewMatrix[2][0]};
    vec3 cameraUpWorld = {ubo.viewMatrix[0][1], ubo.viewMatrix[1][1], ubo.viewMatrix[2][1]};

    PointLight light = ubo.pointLights[gl_InstanceIndex];

    
    vec3 positionWorld = light.position.xyz + 
                        light.radius * fragOffset.x * cameraRightWorld + 
                        light.radius * fragOffset.y * cameraUpWorld;

    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(positionWorld, 1.0);
    fragColor = light.color;
}