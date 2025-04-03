#version 450

layout(location = 0) out vec4 outColor;
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
    PointLight lights[10];
    int lightCount;
    int selectedLight;
    float time;
    
} ubo;
void main() {
    // Create a circle within the point sprite
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(gl_PointCoord, center);

    if (dist > 0.5) {
        discard; // Outside circle
    }

    // Add some shading for 3D appearance
    float lighting = 0.5 + 0.5 * (1.0 - dist * 2.0);
    outColor = vec4(push.color.rgb * lighting, push.color.a);
}
