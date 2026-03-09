#version 330 core

#define NR_POINT_LIGHTS 4

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
};

struct DirectionalLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};

in vec3 Normal;
in vec3 FragmentPosition;
in vec2 TextureCoords;

out vec4 FragmentColor;

uniform vec3 objectColor;
uniform vec3 viewPosition;

uniform DirectionalLight directionalLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

uniform Material material;

vec3 CalculateRegularAmbient(vec3 lightAmbient);
vec3 CalculateRegularDiffuse(vec3 normal, vec3 lightDirection, vec3 lightDiffuse);
vec3 CalculateRegularSpecular(vec3 normal, vec3 lightDirection, vec3 lightSpecular, vec3 viewDirection);

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection);
vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection);
vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection);

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragmentPosition);

    vec3 result = CalculateDirectionalLight(directionalLight, normal, viewDirection);

    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        result += CalculatePointLight(pointLights[i], normal, FragmentPosition, viewDirection);
    }

    result += CalculateSpotLight(spotLight, normal, FragmentPosition, viewDirection);

    vec3 emission = vec3(0.0);
    if (texture(material.specular, TextureCoords).r == 0.0)
    {
        emission = texture(material.emission, TextureCoords).rgb;
    }

    FragmentColor = vec4(result + emission, 1.0);
}

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection)
{
    vec3 lightDirection = normalize(-light.direction);

    vec3 ambient = CalculateRegularAmbient(light.ambient);
    vec3 diffuse = CalculateRegularDiffuse(normal, lightDirection, light.diffuse);
    vec3 specular = CalculateRegularSpecular(normal, lightDirection, light.specular, viewDirection);

    return (ambient + diffuse + specular);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection)
{
    vec3 lightDirection = normalize(light.position - fragmentPosition);

    vec3 ambient = CalculateRegularAmbient(light.ambient);
    vec3 diffuse = CalculateRegularDiffuse(normal, lightDirection, light.diffuse);
    vec3 specular = CalculateRegularSpecular(normal, lightDirection, light.specular, viewDirection);

    float distance = length(light.position - fragmentPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection)
{
    vec3 lightDirection = normalize(light.position - fragmentPosition);

    vec3 ambient = CalculateRegularAmbient(light.ambient);
    vec3 diffuse = CalculateRegularDiffuse(normal, lightDirection, light.diffuse);
    vec3 specular = CalculateRegularSpecular(normal, lightDirection, light.specular, viewDirection);

    float theta = dot(lightDirection, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular);
}

vec3 CalculateRegularAmbient(vec3 lightAmbient)
{
    return lightAmbient * vec3(texture(material.diffuse, TextureCoords));
}

vec3 CalculateRegularDiffuse(vec3 normal, vec3 lightDirection, vec3 lightDiffuse)
{
    float diffuseImpact = max(dot(normal, lightDirection), 0.0);

    return lightDiffuse * diffuseImpact * vec3(texture(material.diffuse, TextureCoords));
}

vec3 CalculateRegularSpecular(vec3 normal, vec3 lightDirection, vec3 lightSpecular, vec3 viewDirection)
{
    vec3 reflectionDirection = reflect(-lightDirection, normal);

    float specularImpact = pow(max(dot(viewDirection, reflectionDirection), 0.0), material.shininess);

    return lightSpecular * specularImpact * vec3(texture(material.specular, TextureCoords));
}