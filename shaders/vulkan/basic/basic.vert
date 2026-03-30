#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inBarycentric;
layout(location = 3) in vec4 inEmissive;
layout(location = 4) in vec4 inNormal;
layout(location = 5) in vec4 inWorldPosition;
layout(location = 6) in vec4 inMaterial;
layout(location = 7) in vec4 inLighting;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragBarycentric;
layout(location = 2) out vec3 fragLocalPosition;
layout(location = 3) out vec3 fragEmissive;
layout(location = 4) out vec3 fragNormal;
layout(location = 5) out vec3 fragWorldPosition;
layout(location = 6) out vec4 fragMaterial;
layout(location = 7) out vec4 fragLighting;

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

void main()
{
    mat4 viewMatrix = transpose(ambientLighting.viewMatrix);
    mat4 projectionMatrix = transpose(ambientLighting.projectionMatrix);
    gl_Position = projectionMatrix * viewMatrix * vec4(inWorldPosition.xyz, 1.0);
    fragColor = inColor;
    fragBarycentric = inBarycentric;
    fragLocalPosition = inPosition;
    fragEmissive = inEmissive.rgb;
    fragNormal = inNormal.xyz;
    fragWorldPosition = inWorldPosition.xyz;
    fragMaterial = inMaterial;
    fragLighting = inLighting;
}
