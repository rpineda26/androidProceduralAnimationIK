#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float isSelected;
layout(location = 0) out vec4 outColor;
struct PointLight{
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
    PointLight lights[10];
    int lightCount;
    int selectedLight;
    float time;
    
} ubo;
const float PI = 3.14159265359;
void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
        
    if(dis >= 1.0) {
        discard;
    }
    outColor = vec4(fragColor.xyz, 0.5 * (cos(dis*PI) + 1.0));
}