#version 450

layout(location = 4) in vec4 inWorldPosition;

layout(push_constant) uniform ShadowPushConstants
{
    vec4 lightViewProjectionRows[4];
} shadowData;

void main()
{
    vec4 point = vec4(inWorldPosition.xyz, 1.0);
    gl_Position = vec4(
        dot(shadowData.lightViewProjectionRows[0], point),
        dot(shadowData.lightViewProjectionRows[1], point),
        dot(shadowData.lightViewProjectionRows[2], point),
        dot(shadowData.lightViewProjectionRows[3], point));
}
