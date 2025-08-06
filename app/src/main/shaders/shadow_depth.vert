// shadow shader vert
#version 450
//shadow depth vert
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in ivec4 joints;
layout(location = 6) in vec4 weights;

// Add joint matrices for animation
layout(set = 0, binding = 0) uniform JointMatrixBufferObject {
    mat4 jointMatrices[200];
} jmbo;

// Push constants for shadow pass
layout(push_constant) uniform Push {
    mat4 mvpMatrix;     // Model-View-Projection from light's perspective
    vec3 lightPos;      // Light position in world space
    bool isAnimated;    // Whether this object is animated
} push;

// Output to fragment shader
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragLightPos;
layout(location = 2) out float fragLightRadius;

void main() {
    vec4 skinnedPosition = vec4(position, 1.0);
    float lightRadius = 30.0;

    // Apply skinning if needed (same logic as PBR shader)
    if (push.isAnimated) {
        skinnedPosition = vec4(0.0);

        float totalWeight = 0.0;
        for (int i = 0; i < 4; i++) {
            if (weights[i] == 0.0) continue;
            if (joints[i] >= 200) {
                // Invalid joint, use original position
                skinnedPosition = vec4(position, 1.0);
                break;
            }

            totalWeight += weights[i];

            // Blend position
            vec4 localPosition = jmbo.jointMatrices[joints[i]] * vec4(position, 1.0);
            skinnedPosition += localPosition * weights[i];
        }

        // Normalize if we have valid weights
        if (totalWeight > 0.0) {
            skinnedPosition /= totalWeight;
        }
    }

    // Transform vertex to light's clip space
    gl_Position = push.mvpMatrix * skinnedPosition;

    // Pass world position for distance calculations in fragment shader
    // Note: You'll need to multiply by model matrix to get true world position
    // This depends on whether your mvpMatrix includes the model matrix or not
    fragWorldPos = skinnedPosition.xyz;
    fragLightPos = push.lightPos;
    fragLightRadius = lightRadius;
}