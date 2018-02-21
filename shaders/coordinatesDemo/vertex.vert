#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normal;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 vertPos;
layout(location = 3) out vec3 worldNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = (ubo.normal * vec4(inNormal, 0.0)).xyz;
    vec4 vertPos4 = ubo.view * ubo.model * vec4(inPosition, 1.0);
    vertPos = vec3(vertPos4) / vertPos4.w;
    worldNormal = inNormal;
}