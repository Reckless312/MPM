#version 330 core

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

// I hate comments

void main()
{
    // Ambient Step
    float ambientStrength = 0.1;

    vec3 ambient = ambientStrength * lightColor;

    // Diffuse Step
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);

    float diffuseImpact = max(dot(norm, lightDirection), 0.0);

    vec3 diffuse = diffuseImpact * lightColor;

    // Specular
    float specularStrength = 0.5;

    vec3 viewDirection = normalize(viewPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, norm);

    float spec = pow(max(dot(viewDirection, reflectionDirection), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Phong Step
    vec3 result = (ambient + diffuse + specular) * objectColor;

    FragColor = vec4(result, 1.0);
}
