#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform float sphereRadius;
uniform float viewportHeight;

flat out int particleID;
out vec3 eyeSpacePos;

void main()
{
    particleID = gl_VertexID;
    vec4 eyePos = view * vec4(aPos, 1.0);
    eyeSpacePos = eyePos.xyz;
    gl_Position = projection * eyePos;
    gl_PointSize = viewportHeight * projection[1][1] * sphereRadius / (-eyePos.z);
}
