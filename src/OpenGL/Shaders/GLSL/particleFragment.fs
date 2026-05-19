#version 330 core

in vec3 eyeSpacePos;

uniform mat4 projection;
uniform float sphereRadius;
uniform vec3 lightDirEye;

out vec4 FragmentColor;

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

    float diffuse = max(0.0, dot(N, lightDirEye));
    float fresnel = pow(1.0 - abs(N.z), 3.0);
    float light = 0.5 + 0.5 * diffuse + 0.4 * fresnel;
    vec3 shadowColor = vec3(0.55, 0.7, 0.9);
    vec3 litColor = vec3(1.0, 1.0, 1.0);
    FragmentColor = vec4(mix(shadowColor, litColor, light), 1.0);
}
