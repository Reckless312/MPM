#version 330 core

struct Material
{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

struct DirectionalLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 Normal;
in vec3 FragmentPosition;
in vec2 TextureCoords;

out vec4 FragmentColor;

uniform vec3 objectColor;
uniform vec3 viewPosition;

uniform DirectionalLight directionalLight;

uniform Material material;
uniform bool hasDiffuseTexture;

vec3 CalculateRegularAmbient(vec3 lightAmbient);
vec3 CalculateRegularDiffuse(vec3 normal, vec3 lightDirection, vec3 lightDiffuse);
vec3 CalculateRegularSpecular(vec3 normal, vec3 lightDirection, vec3 lightSpecular, vec3 viewDirection);

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection);

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragmentPosition);

    vec3 result = CalculateDirectionalLight(directionalLight, normal, viewDirection);

    FragmentColor = vec4(result, 1.0);
}

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection)
{
    vec3 lightDirection = normalize(-light.direction);

    vec3 ambient = CalculateRegularAmbient(light.ambient);
    vec3 diffuse = CalculateRegularDiffuse(normal, lightDirection, light.diffuse);
    vec3 specular = CalculateRegularSpecular(normal, lightDirection, light.specular, viewDirection);

    return (ambient + diffuse + specular);
}

vec3 CalculateRegularAmbient(vec3 lightAmbient)
{
    vec3 color = objectColor;

    if (hasDiffuseTexture)
    {
        color = vec3(texture(material.texture_diffuse1, TextureCoords));
    }

    return lightAmbient * color;
}

vec3 CalculateRegularDiffuse(vec3 normal, vec3 lightDirection, vec3 lightDiffuse)
{
    float diffuseImpact = max(dot(normal, lightDirection), 0.0);

    vec3 color = objectColor;

    if (hasDiffuseTexture)
    {
        color = vec3(texture(material.texture_diffuse1, TextureCoords));
    }

    return lightDiffuse * diffuseImpact * color;
}

vec3 CalculateRegularSpecular(vec3 normal, vec3 lightDirection, vec3 lightSpecular, vec3 viewDirection)
{
    if (!hasDiffuseTexture)
    {
        return vec3(0.0);
    }

    vec3 reflectionDirection = reflect(-lightDirection, normal);

    float specularImpact = pow(max(dot(viewDirection, reflectionDirection), 0.0), material.shininess);

    return lightSpecular * specularImpact * vec3(texture(material.texture_specular1, TextureCoords));
}
