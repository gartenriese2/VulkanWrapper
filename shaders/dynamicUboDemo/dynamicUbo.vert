#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout (binding = 0) uniform UboView 
{
	mat4 view;
	mat4 proj;
} uboView;

layout (binding = 1) uniform UboInstance 
{
	mat4 model; 
} uboInstance;

layout (location = 0) out vec3 outColor;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	//outColor = inColor;
	outColor = abs(uboInstance.model[1].xyz / 2.0);
	mat4 modelView = uboView.view * uboInstance.model;
	//gl_Position = uboView.proj * modelView * vec4(inPosition.xyz, 1.0);
	gl_Position = uboView.proj * uboView.view * vec4(inPosition.xyz, 1.0);
}