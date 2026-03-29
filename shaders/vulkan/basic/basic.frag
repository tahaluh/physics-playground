#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragEmissive;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPosition;
layout(location = 4) in vec4 fragMaterial;
layout(location = 5) in vec4 fragLighting;
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

struct SpotLight
{
    vec4 positionRange;
    vec4 directionInnerCone;
    vec4 colorIntensity;
    vec4 outerConeCos;
};

layout(set = 0, binding = 0) uniform AmbientUniform
{
    vec4 ambientColorIntensity;
    vec4 cameraWorldPosition;
    vec4 shadowLightMatrixRows[4];
    vec4 shadowParams;
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

layout(std430, set = 0, binding = 3) readonly buffer SpotLightBuffer
{
    uvec4 spotLightMeta;
    SpotLight spotLights[];
};

layout(set = 0, binding = 4) uniform sampler2D shadowMap;

vec4 multiplyShadowMatrix(vec3 worldPosition)
{
    vec4 point = vec4(worldPosition, 1.0);
    return vec4(
        dot(ambientLighting.shadowLightMatrixRows[0], point),
        dot(ambientLighting.shadowLightMatrixRows[1], point),
        dot(ambientLighting.shadowLightMatrixRows[2], point),
        dot(ambientLighting.shadowLightMatrixRows[3], point));
}

float computeDirectionalShadowFactor(vec3 worldPosition, float ndotl)
{
    if (ambientLighting.shadowParams.x < 0.5)
    {
        return 1.0;
    }

    vec4 lightClip = multiplyShadowMatrix(worldPosition);
    if (abs(lightClip.w) < 0.0001)
    {
        return 1.0;
    }

    vec3 lightNdc = lightClip.xyz / lightClip.w;
    vec3 shadowCoord = vec3(lightNdc.xy * 0.5 + 0.5, lightNdc.z);
    if (shadowCoord.x <= 0.0 || shadowCoord.x >= 1.0 || shadowCoord.y <= 0.0 || shadowCoord.y >= 1.0 || shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0)
    {
        return 1.0;
    }

    float bias = max(ambientLighting.shadowParams.y * (1.0 - ndotl), ambientLighting.shadowParams.y * 0.25);
    float closestDepth = texture(shadowMap, shadowCoord.xy).r;
    float visibility = shadowCoord.z - bias > closestDepth ? (1.0 - ambientLighting.shadowParams.z) : 1.0;
    return visibility;
}

void main()
{
    vec3 baseColor = fragColor.rgb;
    if (fragMaterial.z > 0.5)
    {
        outColor = vec4(clamp(baseColor + fragEmissive, 0.0, 1.0), fragColor.a);
        return;
    }

    vec3 result = baseColor * ambientLighting.ambientColorIntensity.rgb * ambientLighting.ambientColorIntensity.a * fragMaterial.x + fragEmissive;

    vec3 normal = fragNormal;
    float normalLength = length(normal);
    if (normalLength > 0.0001 && fragMaterial.y > 0.0)
    {
        normal /= normalLength;
        vec3 viewDirection = normalize(ambientLighting.cameraWorldPosition.xyz - fragWorldPosition);
        float specularStrength = fragLighting.x;
        float shininess = max(fragLighting.y, 1.0);
        int lightCount = int(directionalLightMeta.x);
        for (int i = 0; i < lightCount; ++i)
        {
            vec3 lightDirection = normalize(directionalLights[i].direction.xyz);
            float ndotlRaw = dot(normal, -lightDirection);
            float ndotl = fragMaterial.w > 0.5 ? abs(ndotlRaw) : max(ndotlRaw, 0.0);
            float intensity = directionalLights[i].colorIntensity.a;
            vec3 lightColor = directionalLights[i].colorIntensity.rgb;
            float shadowFactor = i == 0 ? computeDirectionalShadowFactor(fragWorldPosition, ndotl) : 1.0;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y * shadowFactor;
            if (ndotl > 0.0 && specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity * shadowFactor;
            }
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
            float ndotlRaw = dot(normal, lightDirection);
            float ndotl = fragMaterial.w > 0.5 ? abs(ndotlRaw) : max(ndotlRaw, 0.0);
            if (ndotl <= 0.0)
            {
                continue;
            }

            float falloff = 1.0 - (distance / range);
            falloff *= falloff;
            float intensity = pointLights[i].colorIntensity.a * falloff;
            vec3 lightColor = pointLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
            if (specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(-lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity;
            }
        }

        int spotLightCount = int(spotLightMeta.x);
        for (int i = 0; i < spotLightCount; ++i)
        {
            vec3 toLight = spotLights[i].positionRange.xyz - fragWorldPosition;
            float distance = length(toLight);
            float range = max(spotLights[i].positionRange.w, 0.0001);
            if (distance >= range)
            {
                continue;
            }

            vec3 lightDirection = distance > 0.0001 ? (toLight / distance) : vec3(0.0, 1.0, 0.0);
            vec3 spotForward = normalize(spotLights[i].directionInnerCone.xyz);
            float spotCos = dot(-lightDirection, spotForward);
            float innerConeCos = spotLights[i].directionInnerCone.w;
            float outerConeCos = spotLights[i].outerConeCos.x;
            if (spotCos <= outerConeCos)
            {
                continue;
            }

            float coneAttenuation = clamp((spotCos - outerConeCos) / max(innerConeCos - outerConeCos, 0.0001), 0.0, 1.0);
            float ndotlRaw = dot(normal, lightDirection);
            float ndotl = fragMaterial.w > 0.5 ? abs(ndotlRaw) : max(ndotlRaw, 0.0);
            if (ndotl <= 0.0)
            {
                continue;
            }

            float falloff = 1.0 - (distance / range);
            falloff *= falloff;
            float intensity = spotLights[i].colorIntensity.a * falloff * coneAttenuation;
            vec3 lightColor = spotLights[i].colorIntensity.rgb;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
            if (specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(-lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity;
            }
        }
    }

    outColor = vec4(clamp(result, 0.0, 1.0), fragColor.a);
}
