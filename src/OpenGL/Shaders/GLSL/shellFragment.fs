#version 330 core

in vec3 eyeSpacePos;

uniform mat4 projection;
uniform float sphereRadius;
uniform vec3 lightDirEye;
uniform sampler2DArray shellColorTextures;
uniform sampler2DArray shellSpecularTextures;
uniform int currentShell;
uniform int totalShells;

out vec4 FragmentColor;

const float pi = 3.14159265358979;
const float alphaThreshold = 0.5;
const float ambientStrength = 0.4;
const float glitterShininess = 16.0;
const float glitterStrength = 0.2;

const vec3 shadowColor = vec3(0.45, 0.60, 0.80);
const vec3 litColor = vec3(1.0, 1.0, 1.0);

void main()
{
    vec3 N;
    N.xy = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;
    N.z = -sqrt(1.0 - r2);

    vec4 pixelPos = vec4(eyeSpacePos + N * sphereRadius, 1.0);
    vec4 clipSpacePos = projection * pixelPos;
    gl_FragDepth = clipSpacePos.z / clipSpacePos.w * 0.5 + 0.5;

    float u = fract(atan(N.y, N.x) / (2.0 * pi));
    float v = acos(-N.z) / pi;
    vec3 texCoord = vec3(u, v, float(currentShell));

    vec4 crystalSample = texture(shellColorTextures, texCoord);
    if (crystalSample.a < alphaThreshold) discard;

    float diffuse = max(0.0, dot(N, lightDirEye));
    float light = ambientStrength + (1.0 - ambientStrength) * diffuse;
    float depthFactor = 0.85 + 0.15 * float(currentShell) / float(totalShells - 1);

    vec3 encodedNormal = texture(shellSpecularTextures, texCoord).xyz;
    vec3 randomNormal = normalize(encodedNormal * 2.0 - 1.0);
    vec3 viewDir = normalize(-eyeSpacePos);
    vec3 reflectDir = reflect(-lightDirEye, randomNormal);
    float glitter = pow(max(0.0, dot(reflectDir, viewDir)), glitterShininess);

    vec3 result = mix(shadowColor, litColor, light) * depthFactor
                + vec3(glitter * glitterStrength);

    FragmentColor = vec4(result, 1.0);
}
