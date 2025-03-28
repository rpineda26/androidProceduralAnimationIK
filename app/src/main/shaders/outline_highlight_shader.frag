#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};
//camera view plus lighting
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
layout(push_constant) uniform Push {
    mat4 modelMatrix;
} push;

//PBR: specular workflow
void main(){
    outColor = vec4(fragColor, 1.0);

}