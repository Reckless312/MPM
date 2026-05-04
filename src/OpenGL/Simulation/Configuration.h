#ifndef MPM_METHOD_CONFIGURATION_H
#define MPM_METHOD_CONFIGURATION_H

class Configuration
{
public:
    int maxBlocks = 16384;
    int cellCountPerAxis = 256;
    float cellSize = 0.02f;
    float deltaTime = 2e-4f;
    float gravity = 9.8f;
    float firstLameParameter = 1.333e4f;
    float secondLameParameter = 2.0e4f;
    float hardeningCoefficient = 10.0f;
    float criticalCompression = 0.025f;
    float criticalStretch = 0.0075f;
    float boundaryFriction = 0.8f;
};

#endif
