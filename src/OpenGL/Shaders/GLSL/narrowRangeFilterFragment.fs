#version 330 core

uniform sampler2D depthTexture;
uniform vec2 blurDir;
uniform vec2 resolution;
uniform float sigma;
uniform float delta;
uniform float mu;
uniform float tanHalfFov;

out vec4 FragmentColor;

void main()
{
    vec2 texCoord = gl_FragCoord.xy / resolution;
    float zi = texture(depthTexture, texCoord).r;

    if (zi == 0.0) discard;

    float sigmaI = max(1.0, floor(resolution.y * sigma / (2.0 * (-zi) * tanHalfFov)));
    float filterRadius = ceil(3.0 * sigmaI);

    float deltaLow = delta;
    float deltaHigh = delta;

    float sum = zi;
    float weightSum = 1.0;

    for (float d = 1.0; d <= filterRadius; d += 1.0)
    {
        float zj = texture(depthTexture, texCoord + d * blurDir).r;
        float zk = texture(depthTexture, texCoord - d * blurDir).r;

        if (zj > zi + deltaHigh || zk > zi + deltaHigh) continue;

        float spatialWeight = exp(-(d * d) / (2.0 * sigmaI * sigmaI));

        float fj = (zj >= zi - deltaLow) ? zj : (zi - mu);
        float fk = (zk >= zi - deltaLow) ? zk : (zi - mu);

        sum += (fj + fk) * spatialWeight;
        weightSum += 2.0 * spatialWeight;

        if (zj != 0.0 && zj <= zi + deltaHigh)
        {
            deltaLow  = max(deltaLow,  zi - zj + delta);
            deltaHigh = max(deltaHigh, zj - zi + delta);
        }
        if (zk != 0.0 && zk <= zi + deltaHigh)
        {
            deltaLow  = max(deltaLow,  zi - zk + delta);
            deltaHigh = max(deltaHigh, zk - zi + delta);
        }
    }

    FragmentColor = vec4(sum / weightSum, 0.0, 0.0, 1.0);
}
