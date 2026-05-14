#version 330 core

uniform sampler2D depthTexture;
uniform vec2 resolution;
uniform float delta;
uniform float mu;

out vec4 FragmentColor;

void main()
{
    vec2 texCoord = gl_FragCoord.xy / resolution;
    float zi = texture(depthTexture, texCoord).r;

    if (zi == 0.0) discard;

    float sum = 0.0;
    float weightSum = 0.0;

    for (float dy = -2.0; dy <= 2.0; dy += 1.0)
    {
        for (float dx = -2.0; dx <= 2.0; dx += 1.0)
        {
            vec2 sampleCoord = texCoord + vec2(dx, dy) / resolution;
            float zj = texture(depthTexture, sampleCoord).r;

            if (zj > zi + delta) continue;

            float spatialWeight = exp(-(dx * dx + dy * dy) / 2.0);

            float fj = (zj >= zi - delta) ? zj : (zi - mu);

            sum += fj * spatialWeight;
            weightSum += spatialWeight;
        }
    }

    FragmentColor = vec4((weightSum > 0.0) ? sum / weightSum : zi, 0.0, 0.0, 1.0);
}
