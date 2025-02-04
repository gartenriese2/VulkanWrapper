#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 vertPos;
layout(location = 3) in vec3 worldNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragNormal, 1.0);
}