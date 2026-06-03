#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform float sphereRadius;
uniform float viewportHeight;

out vec3 viewSpacePosition;

void main()
{
    vec4 viewSpacePosition4 = view * vec4(aPos, 1.0);
    viewSpacePosition = viewSpacePosition4.xyz;
    gl_Position = projection * viewSpacePosition4;
    gl_PointSize = viewportHeight * projection[1][1] * sphereRadius / (-viewSpacePosition4.z);
}
