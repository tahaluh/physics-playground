#version 450

const int POINT_SHADOW_FACE_COUNT = 6;
const float PI = 3.14159265359;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragBarycentric;
layout(location = 2) in vec3 fragLocalPosition;
layout(location = 3) in vec3 fragEmissive;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec3 fragWorldPosition;
layout(location = 6) in vec4 fragMaterial;
layout(location = 7) in vec4 fragLighting;
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
    vec4 shadowBiases;
    vec4 shadowCounts;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 debugFlags;
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

layout(std430, set = 0, binding = 4) readonly buffer DirectionalShadowMatrixBuffer
{
    vec4 directionalShadowMatrixRows[];
};

layout(std430, set = 0, binding = 5) readonly buffer SpotShadowMatrixBuffer
{
    vec4 spotShadowMatrixRows[];
};

layout(std430, set = 0, binding = 6) readonly buffer PointShadowMatrixBuffer
{
    vec4 pointShadowMatrixRows[];
};

layout(set = 0, binding = 7) uniform sampler2DArray directionalShadowMap;
layout(set = 0, binding = 8) uniform sampler2DArray spotShadowMap;
layout(set = 0, binding = 9) uniform sampler2DArray pointShadowMap;

vec4 multiplyShadowMatrix(vec4 shadowRows[4], vec3 worldPosition)
{
    vec4 point = vec4(worldPosition, 1.0);
    return vec4(
        dot(shadowRows[0], point),
        dot(shadowRows[1], point),
        dot(shadowRows[2], point),
        dot(shadowRows[3], point));
}

float sampleShadowPCF4Tap(sampler2DArray depthTexture, vec2 uv, int layerIndex, float compareDepth, float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(depthTexture, 0).xy);
    vec2 offsets[4] = vec2[](
        vec2(-0.5, -0.5),
        vec2(0.5, -0.5),
        vec2(-0.5, 0.5),
        vec2(0.5, 0.5));

    float visibility = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        float closestDepth = texture(depthTexture, vec3(uv + offsets[i] * texelSize, float(layerIndex))).r;
        visibility += compareDepth - bias > closestDepth ? 0.0 : 1.0;
    }

    return visibility * 0.25;
}

float sampleShadowPCF9Tap(sampler2DArray depthTexture, vec2 uv, int layerIndex, float compareDepth, float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(depthTexture, 0).xy);
    float visibility = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float closestDepth = texture(depthTexture, vec3(uv + vec2(float(x), float(y)) * texelSize, float(layerIndex))).r;
            visibility += compareDepth - bias > closestDepth ? 0.0 : 1.0;
        }
    }

    return visibility / 9.0;
}

float computeShadowFactorFromClip(
    sampler2DArray depthTexture,
    int layerIndex,
    vec4 lightClip,
    float ndotl,
    float biasBase,
    bool highQualityFilter)
{
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
    return highQualityFilter
        ? sampleShadowPCF9Tap(depthTexture, shadowCoord.xy, layerIndex, shadowCoord.z, bias)
        : sampleShadowPCF4Tap(depthTexture, shadowCoord.xy, layerIndex, shadowCoord.z, bias);
}

float computeDirectionalShadowFactor(int shadowIndex, vec3 worldPosition, float ndotl)
{
    if (shadowIndex >= int(ambientLighting.shadowCounts.x))
    {
        return 1.0;
    }

    vec4 point = vec4(worldPosition, 1.0);
    vec4 lightClip = vec4(
        dot(directionalShadowMatrixRows[shadowIndex * 4 + 0], point),
        dot(directionalShadowMatrixRows[shadowIndex * 4 + 1], point),
        dot(directionalShadowMatrixRows[shadowIndex * 4 + 2], point),
        dot(directionalShadowMatrixRows[shadowIndex * 4 + 3], point));
    float cameraDistance = length(worldPosition - ambientLighting.cameraWorldPosition.xyz);
    bool highQualityFilter = cameraDistance < 18.0;
    return computeShadowFactorFromClip(
        directionalShadowMap,
        shadowIndex,
        lightClip,
        ndotl,
        ambientLighting.shadowBiases.x,
        highQualityFilter);
}

float computeSpotShadowFactor(int shadowIndex, vec3 worldPosition, float ndotl)
{
    if (shadowIndex >= int(ambientLighting.shadowCounts.z))
    {
        return 1.0;
    }

    vec4 point = vec4(worldPosition, 1.0);
    vec4 lightClip = vec4(
        dot(spotShadowMatrixRows[shadowIndex * 4 + 0], point),
        dot(spotShadowMatrixRows[shadowIndex * 4 + 1], point),
        dot(spotShadowMatrixRows[shadowIndex * 4 + 2], point),
        dot(spotShadowMatrixRows[shadowIndex * 4 + 3], point));
    return computeShadowFactorFromClip(
        spotShadowMap,
        shadowIndex,
        lightClip,
        ndotl,
        ambientLighting.shadowBiases.z,
        false);
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

vec4 multiplyPointShadowMatrix(int pointShadowIndex, int faceIndex, vec3 worldPosition)
{
    vec4 point = vec4(worldPosition, 1.0);
    int baseRowIndex = (pointShadowIndex * POINT_SHADOW_FACE_COUNT + faceIndex) * 4;
    return vec4(
        dot(pointShadowMatrixRows[baseRowIndex + 0], point),
        dot(pointShadowMatrixRows[baseRowIndex + 1], point),
        dot(pointShadowMatrixRows[baseRowIndex + 2], point),
        dot(pointShadowMatrixRows[baseRowIndex + 3], point));
}

float computePointShadowFactor(int pointShadowIndex, vec3 worldPosition, float ndotl)
{
    if (pointShadowIndex >= int(ambientLighting.shadowCounts.y))
    {
        return 1.0;
    }

    vec3 toFragment = worldPosition - pointLights[pointShadowIndex].positionRange.xyz;
    float distanceToLight = length(toFragment);
    if (distanceToLight <= 0.0001 || distanceToLight >= pointLights[pointShadowIndex].positionRange.w)
    {
        return 1.0;
    }

    int faceIndex = selectPointShadowFace(toFragment);
    vec4 lightClip = multiplyPointShadowMatrix(pointShadowIndex, faceIndex, worldPosition);
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
    return sampleShadowPCF4Tap(pointShadowMap, shadowCoord.xy, pointShadowIndex * POINT_SHADOW_FACE_COUNT + faceIndex, shadowCoord.z, bias);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    return numerator / max(denominator, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;
    return numerator / max(denominator, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float computeWireframeMask(vec3 barycentric)
{
    vec3 delta = fwidth(barycentric);
    vec3 edge = smoothstep(vec3(0.0), delta * 1.2, barycentric);
    return 1.0 - min(min(edge.x, edge.y), edge.z);
}

float computeCubeFaceWireframeMask(vec3 localPosition)
{
    vec3 absPos = abs(localPosition);
    float maxAxis = max(absPos.x, max(absPos.y, absPos.z));
    vec2 faceUv;

    if (absPos.x >= absPos.y && absPos.x >= absPos.z)
    {
        faceUv = localPosition.yz;
    }
    else if (absPos.y >= absPos.z)
    {
        faceUv = localPosition.xz;
    }
    else
    {
        faceUv = localPosition.xy;
    }

    vec2 edgeDistance = vec2(0.5) - abs(faceUv);
    vec2 delta = fwidth(faceUv);
    float minDistance = min(edgeDistance.x, edgeDistance.y);
    float threshold = max(max(delta.x, delta.y) * 1.5, 0.001);
    return 1.0 - smoothstep(0.0, threshold, minDistance);
}

vec3 evaluatePbrLight(
    vec3 albedo,
    vec3 lightColor,
    vec3 normal,
    vec3 viewDirection,
    vec3 lightDirection,
    float metallic,
    float roughness,
    float attenuation,
    float diffuseFactor,
    float shadowFactor)
{
    vec3 halfVector = normalize(viewDirection + lightDirection);
    float NdotL = max(dot(normal, lightDirection), 0.0);
    float NdotV = max(dot(normal, viewDirection), 0.0);
    float HdotV = max(dot(halfVector, viewDirection), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(HdotV, F0);
    float NDF = distributionGGX(normal, halfVector, roughness);
    float G = geometrySmith(normal, viewDirection, lightDirection, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;
    vec3 radiance = lightColor * attenuation * shadowFactor;
    return (diffuse * diffuseFactor + specular) * radiance * NdotL;
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
        float metallic = clamp(fragLighting.x, 0.0, 1.0);
        float roughness = clamp(fragLighting.y, 0.045, 1.0);
        int lightCount = int(directionalLightMeta.x);
        for (int i = 0; i < lightCount; ++i)
        {
            vec3 lightDirection = normalize(directionalLights[i].direction.xyz);
            float ndotlRaw = dot(normal, -lightDirection);
            float ndotl = fragMaterial.w > 0.5 ? abs(ndotlRaw) : max(ndotlRaw, 0.0);
            float intensity = directionalLights[i].colorIntensity.a;
            vec3 lightColor = directionalLights[i].colorIntensity.rgb;
            float shadowFactor = i < int(ambientLighting.shadowCounts.x) ? computeDirectionalShadowFactor(i, fragWorldPosition, ndotl) : 1.0;
            if (ndotl > 0.0)
            {
                result += evaluatePbrLight(
                    baseColor,
                    lightColor,
                    normal,
                    viewDirection,
                    -lightDirection,
                    metallic,
                    roughness,
                    intensity,
                    fragMaterial.y,
                    shadowFactor);
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
            float shadowFactor = i < int(ambientLighting.shadowCounts.y) ? computePointShadowFactor(i, fragWorldPosition, ndotl) : 1.0;
            result += evaluatePbrLight(
                baseColor,
                lightColor,
                normal,
                viewDirection,
                lightDirection,
                metallic,
                roughness,
                intensity,
                fragMaterial.y,
                shadowFactor);
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
            float shadowFactor = i < int(ambientLighting.shadowCounts.z) ? computeSpotShadowFactor(i, fragWorldPosition, ndotl) : 1.0;
            result += evaluatePbrLight(
                baseColor,
                lightColor,
                normal,
                viewDirection,
                lightDirection,
                metallic,
                roughness,
                intensity,
                fragMaterial.y,
                shadowFactor);
        }
    }

    vec3 finalColor = clamp(result, 0.0, 1.0);
    if (fragLighting.z > 0.5 || ambientLighting.debugFlags.x > 0.5)
    {
        float wireframeMask = fragLighting.w > 0.5
            ? computeCubeFaceWireframeMask(fragLocalPosition)
            : computeWireframeMask(fragBarycentric);
        finalColor = mix(finalColor, vec3(1.0), wireframeMask);
    }

    outColor = vec4(finalColor, fragColor.a);
}
