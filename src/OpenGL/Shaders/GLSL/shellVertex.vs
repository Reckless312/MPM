#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform float sphereRadius;
uniform float viewportHeight;

flat out float randomAngle;
out vec3 eyeSpacePos;

const float pi = 3.14159265358979;
const float positionHashScale = 10.0;

float PositionHash(vec3 pos)
{
    ivec3 cell = ivec3(floor(pos * positionHashScale));
    uint n = uint(cell.x * 73856093 + cell.y * 19349663 + cell.z * 83492791);
    n = n ^ (n >> 16u);
    n *= 0x45d9f3bu;
    n = n ^ (n >> 16u);
    return float(n) / float(0xFFFFFFFFu) * 2.0 * pi;
}

void main()
{
    vec4 eyePos = view * vec4(aPos, 1.0);
    eyeSpacePos = eyePos.xyz;
    gl_Position = projection * eyePos;
    gl_PointSize = viewportHeight * projection[1][1] * sphereRadius / (-eyePos.z);
    randomAngle = PositionHash(aPos);
}
