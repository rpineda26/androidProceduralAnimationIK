#version 450
//vertex input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in ivec4 joints;
layout(location = 6) in vec4 weights;

//outputs to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;

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


layout(set = 1, binding = 0) uniform JointMatrixBufferObject {
    mat4 jointMatrices[200];
} jmbo;
layout(set = 2, binding = 0) uniform sampler2D albedoSampler;
layout(set = 3, binding = 0) uniform samplerCubeShadow shadowCubeMaps[2];

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    uint textureIndex;
    float smoothness;
//    vec3 baseColor;
    bool isAnimated;
} push;

void main() {
    mat4 skinMatrix = mat4(1.0);
    vec4 skinnedPosition = vec4(position, 1.0);
    vec3 skinnedNormal = normal;

    fragColor = color;

    // Apply skinning if needed
    if (push.isAnimated) {
        skinMatrix = mat4(0.0);
        skinnedPosition = vec4(0.0);
        skinnedNormal = vec3(0.0);

        float totalWeight = 0.0;
        for (int i = 0; i < 4; i++) {
            if (weights[i] == 0.0) continue;
            if (joints[i] >= 200) {
                // Invalid joint, use identity
                skinMatrix = mat4(1.0);
                skinnedPosition = vec4(position, 1.0);
                skinnedNormal = normal;
                break;
            }

            totalWeight += weights[i];

            // Check for problematic joint matrices
            if (determinant(mat3(jmbo.jointMatrices[joints[i]])) < 0.0) {
                // Debug: color problematic joints red
                fragColor = vec3(1.0, 0.0, 0.0);
            }

            // Blend position and normal
            vec4 localPosition = jmbo.jointMatrices[joints[i]] * vec4(position, 1.0);
            vec3 localNormal = mat3(jmbo.jointMatrices[joints[i]]) * normal;

            skinnedPosition += localPosition * weights[i];
            skinnedNormal += localNormal * weights[i];
            skinMatrix += jmbo.jointMatrices[joints[i]] * weights[i];
        }

        // Normalize if we have valid weights
        if (totalWeight > 0.0) {
            skinnedPosition /= totalWeight;
            skinnedNormal = normalize(skinnedNormal);
        }
    }

    // Transform to world space
    vec4 positionWorld = push.modelMatrix * skinnedPosition;
    fragPosWorld = positionWorld.xyz;

    // Transform normal to world space
    // Use the combined model + skin matrix for normal transformation
    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix * skinMatrix)));
    fragNormalWorld = normalize(normalMatrix * skinnedNormal);

    // Final vertex position
    gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionWorld);

    // Pass through UV coordinates
    fragUV = uv;
}