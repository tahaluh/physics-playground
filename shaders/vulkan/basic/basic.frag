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
            result += baseColor * lightColor * ndotl * intensity * fragMaterial.y;
            if (ndotl > 0.0 && specularStrength > 0.0)
            {
                vec3 reflectDirection = reflect(lightDirection, normal);
                float specular = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
                result += lightColor * specular * specularStrength * intensity;
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
