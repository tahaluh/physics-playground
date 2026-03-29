#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragEmissive;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPosition;
layout(location = 4) in vec2 fragMaterial;
layout(location = 0) out vec4 outColor;

const int MAX_DIRECTIONAL_LIGHTS = 4;
const int MAX_POINT_LIGHTS = 8;

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

layout(set = 0, binding = 0) uniform LightingUniform
{
    vec4 ambientColorIntensity;
    vec4 directionalLightMeta;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    vec4 pointLightMeta;
    PointLight pointLights[MAX_POINT_LIGHTS];
} lighting;

void main()
{
    vec3 baseColor = fragColor.rgb;
    vec3 result = baseColor * lighting.ambientColorIntensity.rgb * lighting.ambientColorIntensity.a * fragMaterial.x + fragEmissive;

    vec3 normal = fragNormal;
    float normalLength = length(normal);
    if (normalLength > 0.0001 && fragMaterial.y > 0.0)
    {
        normal /= normalLength;
        int lightCount = min(int(lighting.directionalLightMeta.x + 0.5), MAX_DIRECTIONAL_LIGHTS);
        for (int i = 0; i < lightCount; ++i)
        {
            vec3 lightDirection = normalize(lighting.directionalLights[i].direction.xyz);
            float ndotl = max(dot(normal, -lightDirection), 0.0);
            float intensity = lighting.directionalLights[i].colorIntensity.a;
            vec3 lightColor = lighting.directionalLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
        }

        int pointLightCount = min(int(lighting.pointLightMeta.x + 0.5), MAX_POINT_LIGHTS);
        for (int i = 0; i < pointLightCount; ++i)
        {
            vec3 toLight = lighting.pointLights[i].positionRange.xyz - fragWorldPosition;
            float distance = length(toLight);
            float range = max(lighting.pointLights[i].positionRange.w, 0.0001);
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
            float intensity = lighting.pointLights[i].colorIntensity.a * falloff;
            vec3 lightColor = lighting.pointLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
        }
    }

    outColor = vec4(clamp(result, 0.0, 1.0), fragColor.a);
}
