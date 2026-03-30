#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec4 inModelRow0;
layout(location = 5) in vec4 inModelRow1;
layout(location = 6) in vec4 inModelRow2;
layout(location = 7) in vec4 inModelRow3;

layout(push_constant) uniform ShadowPushConstants
{
    vec4 lightViewProjectionRows[4];
} shadowData;

void main()
{
    mat4 modelMatrix = transpose(mat4(inModelRow0, inModelRow1, inModelRow2, inModelRow3));
    vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);
    gl_Position = vec4(
        dot(shadowData.lightViewProjectionRows[0], worldPosition),
        dot(shadowData.lightViewProjectionRows[1], worldPosition),
        dot(shadowData.lightViewProjectionRows[2], worldPosition),
        dot(shadowData.lightViewProjectionRows[3], worldPosition));
}
