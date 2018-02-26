#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define lightCount 6

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in vec3 vertPos;
layout (location = 2) in vec4 lightInfo;
layout (location = 3) in vec4 inLightVec[lightCount];

layout (location = 0) out vec4 outFragColor;

void main() 
{
    vec3 lightColor[lightCount];
    lightColor[0] = vec3(1.0, 0.0, 0.0);
    lightColor[1] = vec3(0.0, 1.0, 0.0);
    lightColor[2] = vec3(0.0, 0.0, 1.0);
    lightColor[3] = vec3(1.0, 0.0, 1.0);
    lightColor[4] = vec3(0.0, 1.0, 1.0);
    lightColor[5] = vec3(1.0, 1.0, 0.0);

	const float shininess = lightInfo.x;
	const vec3 ambientColor = vec3(lightInfo.y);
	const float screenGamma = lightInfo.z;
	const float maxLightDist = lightInfo.w * lightInfo.w;

    vec3 normal = normalize(fragNormal);

    vec3 diffuse = ambientColor;
    // Just some very basic attenuation
    for (int i = 0; i < lightCount; ++i)
    {
        float lRadius =  maxLightDist * inLightVec[i].w;

        float dist = min(dot(inLightVec[i], inLightVec[i]), lRadius) / lRadius;
        float distFactor = 1.0 - dist;

		vec3 lightDir = normalize(inLightVec[i].xyz);
        float lambertian = max(dot(lightDir, normal), 0.0);
		float specular = 0.0;

		if(lambertian > 0.0)
		{
			vec3 viewDir = normalize(-vertPos);
			vec3 halfDir = normalize(lightDir + viewDir);
			float specAngle = max(dot(halfDir, normal), 0.0);
			specular = pow(specAngle, shininess);
		}

		vec3 colorLinear = lambertian * lightColor[i] * distFactor +
                           specular * lightColor[i] * distFactor;

		diffuse += colorLinear;
    }

	diffuse = pow(diffuse, vec3(1.0 / screenGamma));
    outFragColor.rgb = diffuse;
}