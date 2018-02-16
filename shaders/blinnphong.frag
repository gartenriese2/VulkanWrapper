#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 vertPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const vec3 lightPos = vec3(1.0,1.0,1.0);
const vec3 ambientColor = vec3(0.1, 0.0, 0.0);
const vec3 diffuseColor = vec3(0.5, 0.0, 0.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;
const float screenGamma = 2.2;

void main()
{
    vec3 normal = -normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - vertPos);

    float lambertian = max(dot(lightDir,normal), 0.0);
    float specular = 0.0;

    if(lambertian > 0.0)
    {
        vec3 viewDir = normalize(-vertPos);

        // this is blinn phong
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, shininess);
    }

    vec3 colorLinear = ambientColor +
                       lambertian * diffuseColor +
                       specular * specColor;
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0/screenGamma));

    outColor = vec4(colorGammaCorrected, 1.0);
}