#version 330 core

in vec3 viewSpacePosition;

uniform sampler2DArray shellColorTextures;
uniform sampler2DArray shellSpecularTextures;

uniform mat4 projection;
uniform vec3 lightDirectionView;

uniform float sphereRadius;

uniform int currentShell;
uniform int totalShells;

out vec4 FragmentColor;

void main()
{
    vec3 sphereNormal;
    sphereNormal.xy = gl_PointCoord * 2.0 - 1.0;

    float squaredDistance = dot(sphereNormal.xy, sphereNormal.xy);

    if (squaredDistance > 1.0)
    {
        discard;
    }

    sphereNormal.z = -sqrt(1.0 - squaredDistance);

    vec4 pixelPosition = vec4(viewSpacePosition + sphereNormal * sphereRadius, 1.0);
    vec4 clipSpacePosition = projection * pixelPosition;

    gl_FragDepth = clipSpacePosition.z / clipSpacePosition.w * 0.5 + 0.5;

    const float pi = acos(-1.0);

    float longitude = fract(atan(sphereNormal.y, sphereNormal.x) / (2.0 * pi));
    float latitude = acos(-sphereNormal.z) / pi;

    vec3 textureCoordinates = vec3(longitude, latitude, float(currentShell));

    vec4 crystalSample = texture(shellColorTextures, textureCoordinates);
    const float alphaThreshold = 0.5;

    if (crystalSample.a < alphaThreshold)
    {
        discard;
    }

    float diffuse = max(0.0, dot(sphereNormal, lightDirectionView));

    const float ambientStrength = 0.4;
    float lightIntensity = ambientStrength + (1.0 - ambientStrength) * diffuse;

    const float shellDepthMin = 0.85;
    const float shellDepthRange = 0.15;
    float depthFactor = shellDepthMin + shellDepthRange * float(currentShell) / float(totalShells - 1);

    vec3 encodedNormal = texture(shellSpecularTextures, textureCoordinates).xyz;
    vec3 crystalNormal = normalize(encodedNormal * 2.0 - 1.0);

    vec3 viewDirection = normalize(-viewSpacePosition);
    vec3 reflectDirection = reflect(-lightDirectionView, crystalNormal);

    const float glitterShininess = 16.0;
    const float glitterStrength = 0.2;
    float glitterIntensity = pow(max(0.0, dot(reflectDirection, viewDirection)), glitterShininess);

    const vec3 snowShadowColor = vec3(0.45, 0.60, 0.80);
    const vec3 snowLitColor = vec3(1.0, 1.0, 1.0);
    vec3 result = mix(snowShadowColor, snowLitColor, lightIntensity) * depthFactor + vec3(glitterIntensity * glitterStrength);

    FragmentColor = vec4(result, 1.0);
}
