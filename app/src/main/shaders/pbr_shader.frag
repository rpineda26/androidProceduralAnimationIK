#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragNormalWorld;

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
    PointLight pointLights[10];
    int lightCount;
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
    vec3 baseColor;
    bool isAnimated;
} push;
const float PI = 3.14159265359;
const float MIN_ROUGHNESS = 0.04;
const float smoothness_input_weight = 0.75;

//distribution function
float DistributionGGX(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}
//geometry function
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float alpha = roughness * roughness;
    float k = alpha / 2.0;
    float ggx1 = NdotV * sqrt(NdotL * NdotL * (1.0 - k) + k);
    float ggx2 = NdotL * sqrt(NdotV * NdotV * (1.0 - k) + k);
    return ggx1 * ggx2;
}
//fresnel function
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float sampleShadowCube(samplerCubeShadow shadowMap, vec3 lightPos, vec3 fragPos, float lightRadius) {
    vec3 lightToFrag = fragPos - lightPos;
    float actualDistance = length(lightToFrag);
    float normalizedDistance = actualDistance / lightRadius;

    // The 4th component is the comparison value for shadow testing
    return texture(shadowMap, vec4(normalize(lightToFrag), normalizedDistance));
}

//PBR: specular workflow
void main() {
    // Initialize material properties
    vec4 albedo = vec4(0.9, 0.9, 0.9, 1.0);
    vec3 F0 = vec3(0.04);
    float roughness = clamp(1.0 - push.smoothness, MIN_ROUGHNESS, 1.0);
    // Handle different texture indices
    if (push.textureIndex == 0) {
        float tileSize = 2.0; // Each tile is 2x2 world units
        vec2 worldPos = fragPosWorld.xz; // Use X and Z coordinates
        vec2 tileIndex = floor(worldPos / tileSize);
        float selector = mod(tileIndex.x + tileIndex.y, 2.0);

        vec3 colorA = vec3(0.4, 0.4, 0.4); // Darker Light Gray
        vec3 colorB = vec3(0.25, 0.25, 0.25); // Darker Gray

        if (selector < 1.0) {
            albedo.rgb = colorA;
        } else {
            albedo.rgb = colorB;
        }

        // Adjust material properties for floor
        F0 = vec3(0.04); // Low specular for matte floor
        roughness = 0.8; // High roughness for matte appearance
    } else if (push.textureIndex > 0 && push.textureIndex < 4294967295u) {
        // Sample regular texture
        vec4 texColor = texture(albedoSampler, fragUV);
        albedo.rgb *= texColor.rgb;
        albedo.a = texColor.a;
    }

    // Use world-space normal directly (no normal mapping)
    vec3 N = normalize(fragNormalWorld);
    vec3 V = normalize(vec3(ubo.invViewMatrix[3]) - fragPosWorld);
    float NdotV = max(dot(N, V), 0.0001);

    // PBR lighting calculation with proper shadow handling
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < ubo.lightCount && i < 10; i++) {
        vec3 lightPos = ubo.pointLights[i].position.xyz;
        vec3 lightColor = ubo.pointLights[i].color.rgb;
        float lightIntensity = ubo.pointLights[i].color.w;

        // Light vector and attenuation
        vec3 L = lightPos - fragPosWorld;
        float distance = length(L);
        L = normalize(L);

        // Simple attenuation
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        vec3 radiance = lightColor * lightIntensity * attenuation;

        // Half vector
        vec3 H = normalize(V + L);

        // Dot products with safety clamps
        float NdotL = max(dot(N, L), 0.0001);
        float NdotH = max(dot(N, H), 0.0001);
        float VdotH = max(dot(V, H), 0.0001);

        // BRDF components
        float D = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        vec3 F = FresnelSchlick(VdotH, F0);

        // Specular BRDF with safety check
        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular = numerator / max(denominator, 0.0001);

        // Energy conservation
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;

        // Lambertian diffuse
        vec3 diffuse = kD * albedo.rgb / PI;

        // Sample shadow for this light if available
        float shadow = 1.0;
        if (i < 2) { // Only sample if we have a shadow map for this light
            shadow = sampleShadowCube(shadowCubeMaps[i], lightPos, fragPosWorld, 25.0);
        }

        // Add to outgoing light with shadow applied
        Lo += (diffuse + specular) * radiance * NdotL * shadow;
    }

    // Ambient lighting
    vec3 ambient = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w * albedo.rgb;
    // Ensure minimum ambient so objects are never completely black
    ambient = max(ambient, vec3(0.03) * albedo.rgb);

    vec3 color = ambient + Lo;

    // Simple tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    outColor = vec4(color, albedo.a);
}