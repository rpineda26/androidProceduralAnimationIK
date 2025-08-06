#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

struct PointLight{
    vec4 position;
    vec4 color;
    float radius; //only used in the shader for rendering the point light
    int objId;
};

layout(push_constant) uniform Push {
    mat4 modelMatrix;
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
    // Start with ambient lighting
    vec3 ambientLight = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w * push.color.rgb;
    ambientLight = max(ambientLight, vec3(0.03) * push.color.rgb);
    vec3 diffuseLight = vec3(0.0);
    vec3 normal = normalize(fragNormalWorld);

    // Calculate lighting from all point lights
    for (int i = 0; i < ubo.lightCount && i < 10; i++) {
        vec3 lightPos = ubo.lights[i].position.xyz;
        vec3 lightColor = ubo.lights[i].color.xyz;
        float lightIntensity = ubo.lights[i].color.w;

        // Calculate direction and distance to light
        vec3 directionToLight = lightPos - fragPosWorld;
        float distance = length(directionToLight);

        // Normalize direction
        directionToLight = normalize(directionToLight);

        // Calculate attenuation (inverse square law with minimum distance)
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // Calculate diffuse lighting
        float cosAngIncidence = max(dot(normal, directionToLight), 0.01);

        // Add this light's contribution
        diffuseLight += lightColor * lightIntensity * attenuation * cosAngIncidence;
    }

    vec3 light = ambientLight + diffuseLight;

    vec3 finalColor = fragColor * light;
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0/2.2));
    outColor = vec4(finalColor, push.color.a);
//    vec3 normalColor = normalize(fragNormalWorld) * 0.5 + 0.5; // Convert -1,1 to 0,1
//    outColor = vec4(normalColor, 1.0);

}
