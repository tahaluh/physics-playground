#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragEmissive;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPosition;
layout(location = 4) in vec2 fragMaterial;
layout(location = 0) out vec4 outColor;

struct DirectionalLight
{
    vec4 direction;
    vec4 colorIntensity;
};

struct PointLight
{
    vec4 positionRange;
    vec4 colorIntensity;
};

layout(set = 0, binding = 0) uniform AmbientUniform
{
    vec4 ambientColorIntensity;
} ambientLighting;

layout(std430, set = 0, binding = 1) readonly buffer DirectionalLightBuffer
{
    uvec4 directionalLightMeta;
    DirectionalLight directionalLights[];
};

layout(std430, set = 0, binding = 2) readonly buffer PointLightBuffer
{
    uvec4 pointLightMeta;
    PointLight pointLights[];
};

void main()
{
    vec3 baseColor = fragColor.rgb;
    vec3 result = baseColor * ambientLighting.ambientColorIntensity.rgb * ambientLighting.ambientColorIntensity.a * fragMaterial.x + fragEmissive;

    vec3 normal = fragNormal;
    float normalLength = length(normal);
    if (normalLength > 0.0001 && fragMaterial.y > 0.0)
    {
        normal /= normalLength;
        int lightCount = int(directionalLightMeta.x);
        for (int i = 0; i < lightCount; ++i)
        {
            vec3 lightDirection = normalize(directionalLights[i].direction.xyz);
            float ndotl = max(dot(normal, -lightDirection), 0.0);
            float intensity = directionalLights[i].colorIntensity.a;
            vec3 lightColor = directionalLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
        }

        int pointLightCount = int(pointLightMeta.x);
        for (int i = 0; i < pointLightCount; ++i)
        {
            vec3 toLight = pointLights[i].positionRange.xyz - fragWorldPosition;
            float distance = length(toLight);
            float range = max(pointLights[i].positionRange.w, 0.0001);
            if (distance >= range)
            {
                continue;
            }

            vec3 lightDirection = distance > 0.0001 ? (toLight / distance) : vec3(0.0, 1.0, 0.0);
            float ndotl = max(dot(normal, lightDirection), 0.0);
            if (ndotl <= 0.0)
            {
                continue;
            }

            float falloff = 1.0 - (distance / range);
            falloff *= falloff;
            float intensity = pointLights[i].colorIntensity.a * falloff;
            vec3 lightColor = pointLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
        }
    }

    outColor = vec4(clamp(result, 0.0, 1.0), fragColor.a);
}
