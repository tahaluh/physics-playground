#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inTriangleColor;
layout(location = 2) in vec4 inNormal;
layout(location = 3) in vec4 inModelRow0;
layout(location = 4) in vec4 inModelRow1;
layout(location = 5) in vec4 inModelRow2;
layout(location = 6) in vec4 inModelRow3;
layout(location = 7) in vec4 inColor;
layout(location = 8) in vec4 inEmissive;
layout(location = 9) in vec4 inMaterial;
layout(location = 10) in vec4 inLighting;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragEmissive;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPosition;
layout(location = 4) out vec4 fragMaterial;
layout(location = 5) out vec4 fragLighting;

layout(set = 0, binding = 0) uniform AmbientUniform
{
    vec4 ambientColorIntensity;
    vec4 cameraWorldPosition;
    vec4 shadowBiases;
    vec4 shadowCounts;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} ambientLighting;

void main()
{
    mat4 modelMatrix = transpose(mat4(inModelRow0, inModelRow1, inModelRow2, inModelRow3));
    vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);

    mat4 viewMatrix = transpose(ambientLighting.viewMatrix);
    mat4 projectionMatrix = transpose(ambientLighting.projectionMatrix);
    gl_Position = projectionMatrix * viewMatrix * worldPosition;

    vec3 worldNormal = normalize(mat3(modelMatrix) * inNormal.xyz);
    fragColor = inTriangleColor.a > 0.0 ? inTriangleColor : inColor;
    fragEmissive = inEmissive.rgb;
    fragNormal = worldNormal;
    fragWorldPosition = worldPosition.xyz;
    fragMaterial = inMaterial;
    fragLighting = inLighting;
}
