#version 330 core

uniform sampler2D depthTexture;
uniform vec2 resolution;
uniform mat4 projection;

out vec4 FragmentColor;

vec3 uvToEye(vec2 uv, float depth)
{
    vec2 ndc = uv * 2.0 - 1.0;
    float eyeX = ndc.x * (-depth) / projection[0][0];
    float eyeY = ndc.y * (-depth) / projection[1][1];
    return vec3(eyeX, eyeY, depth);
}

vec3 getEyePos(vec2 uv)
{
    float depth = texture(depthTexture, uv).r;
    return uvToEye(uv, depth);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;
    float depth = texture(depthTexture, uv).r;

    if (depth == 0.0) discard;

    vec2 texelSize = 1.0 / resolution;
    vec3 posEye = uvToEye(uv, depth);

    vec3 ddx = getEyePos(uv + vec2(texelSize.x, 0.0)) - posEye;
    vec3 ddx2 = posEye - getEyePos(uv - vec2(texelSize.x, 0.0));
    if (abs(ddx.z) > abs(ddx2.z)) ddx = ddx2;

    vec3 ddy = getEyePos(uv + vec2(0.0, texelSize.y)) - posEye;
    vec3 ddy2 = posEye - getEyePos(uv - vec2(0.0, texelSize.y));
    if (abs(ddy.z) > abs(ddy2.z)) ddy = ddy2;

    vec3 normal = normalize(cross(ddx, ddy));

    FragmentColor = vec4(normal * 0.5 + 0.5, 1.0);
}
