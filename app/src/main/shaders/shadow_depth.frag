#version 450
//shadow depth frag
// Input from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragLightPos;
layout(location = 2) in float fragLightRadius;


void main() {
    // Calculate distance from fragment to light
    float distance = length(fragWorldPos - fragLightPos);

    // Normalize distance by light radius (0.0 to 1.0 range)
    float normalizedDistance = distance / fragLightRadius;

    // Clamp to valid depth range
    normalizedDistance = clamp(normalizedDistance, 0.0, 1.0);

    // Write normalized distance to depth buffer
    // Vulkan uses reverse Z, so closer fragments have higher depth values
    gl_FragDepth = normalizedDistance;

    // No color output needed for depth-only rendering
}