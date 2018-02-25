#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

#define lightCount 6

layout (binding = 0) uniform UBO 
{
	mat4 model;
	mat4 view;
	mat4 projection;
	//vec4 lightColor[lightCount];
} ubo;

layout(push_constant) uniform PushConsts {
	vec4 lightPos[lightCount];
} pushConsts;

layout (location = 0) out vec4 outLightVec[lightCount];

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
	
	for (int i = 0; i < lightCount; ++i)
	{	
		vec4 worldPos =  ubo.model * vec4(inPos.xyz, 1.0);
		outLightVec[i].xyz = pushConsts.lightPos[i].xyz - inPos.xyz;		
		// Store light radius in w
		outLightVec[i].w = pushConsts.lightPos[i].w;
	}
}