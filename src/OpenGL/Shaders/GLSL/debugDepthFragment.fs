#version 330 core

uniform sampler2D depthTexture;
uniform vec2 resolution;

out vec4 FragmentColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;
    float depth = texture(depthTexture, uv).r;

    if (depth == 0.0) discard;

    float normalized = 1.0 - clamp(-depth / 8.0, 0.0, 1.0);
    FragmentColor = vec4(vec3(normalized), 1.0);
}
