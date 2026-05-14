#version 330 core

uniform sampler2D depthTexture;
uniform vec2 blurDir;
uniform float filterRadius;
uniform float blurScale;
uniform float blurDepthFalloff;

out vec4 FragmentColor;

void main()
{
    vec2 texCoord = gl_FragCoord.xy / vec2(textureSize(depthTexture, 0));
    float depth = texture(depthTexture, texCoord).r;

    if (depth == 0.0) discard;

    float sum = 0.0;
    float weightSum = 0.0;

    for (float offset = -filterRadius; offset <= filterRadius; offset += 1.0)
    {
        float sampleDepth = texture(depthTexture, texCoord + offset * blurDir).r;

        float spatialWeight = exp(-(offset * blurScale) * (offset * blurScale));

        float depthDiff = (sampleDepth - depth) * blurDepthFalloff;
        float rangeWeight = exp(-depthDiff * depthDiff);

        float weight = spatialWeight * rangeWeight;
        sum += sampleDepth * weight;
        weightSum += weight;
    }

    if (weightSum > 0.0)
    {
        sum /= weightSum;
    }

    FragmentColor = vec4(sum, 0.0, 0.0, 1.0);
}
