#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inEmissive;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inWorldPosition;
layout(location = 5) in vec4 inMaterial;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragEmissive;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPosition;
layout(location = 4) out vec2 fragMaterial;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
    fragEmissive = inEmissive.rgb;
    fragNormal = inNormal.xyz;
    fragWorldPosition = inWorldPosition.xyz;
    fragMaterial = inMaterial.xy;
}
