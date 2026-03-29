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
    vec4 directionalShadowMatrixRows[4];
    vec4 spotShadowMatrixRows[4];
    vec4 pointShadowMatrixRows[24];
    vec4 shadowFlags;
    vec4 shadowBiases;
    vec4 pointShadowPositionRange;
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

layout(set = 0, binding = 4) uniform sampler2D directionalShadowMap;
layout(set = 0, binding = 5) uniform sampler2D spotShadowMap;
layout(set = 0, binding = 6) uniform sampler2DArray pointShadowMap;

vec4 multiplyShadowMatrix(vec4 shadowRows[4], vec3 worldPosition)
{
    vec4 point = vec4(worldPosition, 1.0);
    return vec4(
        dot(shadowRows[0], point),
        dot(shadowRows[1], point),
        dot(shadowRows[2], point),
        dot(shadowRows[3], point));
}

float sampleShadowPCF(sampler2D depthTexture, vec2 uv, float compareDepth, float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(depthTexture, 0));
    float visibility = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 offsetUv = uv + vec2(float(x), float(y)) * texelSize;
            float closestDepth = texture(depthTexture, offsetUv).r;
            visibility += compareDepth - bias > closestDepth ? 0.0 : 1.0;
        }
    }

    return visibility / 9.0;
}

float samplePointShadowPCF(sampler2DArray depthTexture, vec2 uv, int faceIndex, float compareDepth, float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(depthTexture, 0).xy);
    float visibility = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 offsetUv = uv + vec2(float(x), float(y)) * texelSize;
            float closestDepth = texture(depthTexture, vec3(offsetUv, float(faceIndex))).r;
            visibility += compareDepth - bias > closestDepth ? 0.0 : 1.0;
        }
    }

    return visibility / 9.0;
}

float computeShadowFactor(sampler2D depthTexture, vec4 shadowRows[4], vec3 worldPosition, float ndotl, float biasBase)
{
    vec4 lightClip = multiplyShadowMatrix(shadowRows, worldPosition);
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

    float bias = max(biasBase * (1.0 - ndotl), biasBase * 0.35);
    return sampleShadowPCF(depthTexture, shadowCoord.xy, shadowCoord.z, bias);
}

float computeDirectionalShadowFactor(vec3 worldPosition, float ndotl)
{
    if (ambientLighting.shadowFlags.x < 0.5)
    {
        return 1.0;
    }

    return computeShadowFactor(
        directionalShadowMap,
        ambientLighting.directionalShadowMatrixRows,
        worldPosition,
        ndotl,
        ambientLighting.shadowBiases.x);
}

float computeSpotShadowFactor(vec3 worldPosition, float ndotl)
{
    if (ambientLighting.shadowFlags.z < 0.5)
    {
        return 1.0;
    }

    return computeShadowFactor(
        spotShadowMap,
        ambientLighting.spotShadowMatrixRows,
        worldPosition,
        ndotl,
        ambientLighting.shadowBiases.z);
}

int selectPointShadowFace(vec3 directionFromLight)
{
    vec3 axis = abs(directionFromLight);
    if (axis.x > axis.y && axis.x > axis.z)
    {
        return directionFromLight.x > 0.0 ? 0 : 1;
    }
    if (axis.y > axis.z)
    {
        return directionFromLight.y > 0.0 ? 2 : 3;
    }
    return directionFromLight.z > 0.0 ? 4 : 5;
}

vec4 multiplyPointShadowMatrix(int faceIndex, vec3 worldPosition)
{
    vec4 shadowRows[4] = vec4[](
        ambientLighting.pointShadowMatrixRows[faceIndex * 4 + 0],
        ambientLighting.pointShadowMatrixRows[faceIndex * 4 + 1],
        ambientLighting.pointShadowMatrixRows[faceIndex * 4 + 2],
        ambientLighting.pointShadowMatrixRows[faceIndex * 4 + 3]);
    return multiplyShadowMatrix(shadowRows, worldPosition);
}

float computePointShadowFactor(vec3 worldPosition, float ndotl)
{
    if (ambientLighting.shadowFlags.y < 0.5)
    {
        return 1.0;
    }

    vec3 toFragment = worldPosition - ambientLighting.pointShadowPositionRange.xyz;
    float distanceToLight = length(toFragment);
    if (distanceToLight <= 0.0001 || distanceToLight >= ambientLighting.pointShadowPositionRange.w)
    {
        return 1.0;
    }

    int faceIndex = selectPointShadowFace(toFragment);
    vec4 lightClip = multiplyPointShadowMatrix(faceIndex, worldPosition);
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

    float bias = max(ambientLighting.shadowBiases.y * (1.0 - ndotl), ambientLighting.shadowBiases.y * 0.35);
    return samplePointShadowPCF(pointShadowMap, shadowCoord.xy, faceIndex, shadowCoord.z, bias);
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
            float shadowFactor = i == 0 ? computePointShadowFactor(fragWorldPosition, ndotl) : 1.0;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y * shadowFactor;
            if (specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(-lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity * shadowFactor;
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
            float shadowFactor = i == 0 ? computeSpotShadowFactor(fragWorldPosition, ndotl) : 1.0;
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y * shadowFactor;
            if (specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(-lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity * shadowFactor;
            }
        }
    }

    outColor = vec4(clamp(result, 0.0, 1.0), fragColor.a);
}
