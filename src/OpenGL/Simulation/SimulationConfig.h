#ifndef MPM_METHOD_SIMULATIONCONFIG_H
#define MPM_METHOD_SIMULATIONCONFIG_H

#include <cmath>
#include <iostream>

#include "../Program.h"

namespace SimulationConfig
{
    inline constexpr int maxBlocks = 16384;
    inline constexpr int cellCountPerAxis = 256;
    inline constexpr int nodeCount = cellCountPerAxis * cellCountPerAxis * cellCountPerAxis;
    inline constexpr float cellSize = 0.02f;
    inline constexpr float gridSize = cellCountPerAxis * cellSize;

    inline static float physicsTimeStep = 3e-4f;
    //inline static float physicsTimeStep = 1e-4f;
    //inline static float physicsTimeStep = 5e-5f;

    inline float firstLameParameter = 3.889e4f;
    inline float secondLameParameter = 5.833e4f;
    inline float hardeningCoefficient = 10.0f;
    inline float criticalCompression = 0.025f;
    inline float criticalStretch = 0.0075f;
    inline float boundaryFriction = 0.8f;
    
    inline int defaultSimulationSubsteps = 6;
    inline int simulationSubsteps = Program::recordingMode ? static_cast<int>(std::roundf(1.0f / (static_cast<float>(Program::recordingFrameRate) * SimulationConfig::physicsTimeStep))) : defaultSimulationSubsteps;

    inline void UpdateParameters(const float newFirstLameParameter, const float newSecondLameParameter, const float newHardeningCoefficient, const float newCriticalCompression, const float newCriticalStretch, const float newBoundaryFriction)
    {
        SimulationConfig::firstLameParameter = newFirstLameParameter;
        SimulationConfig::secondLameParameter = newSecondLameParameter;
        SimulationConfig::hardeningCoefficient = newHardeningCoefficient;
        SimulationConfig::criticalCompression = newCriticalCompression;
        SimulationConfig::criticalStretch = newCriticalStretch;
        SimulationConfig::boundaryFriction = newBoundaryFriction;
    }

    inline void SwitchScenesParameters(const int scene)
    {
        switch (scene)
        {
            case 1:
                SimulationConfig::UpdateParameters(3.889e4f, 5.833e4f, 10.0f, 0.025f, 0.0075f, 0.8f);
                break;
            case 2:
                SimulationConfig::UpdateParameters(0.75e4f, 1.25e4f, 1.5f, 0.025f, 0.0075f, 0.3f);
                break;
            case 3:
                SimulationConfig::UpdateParameters(1.333e4f, 2.0e4f, 1.0f, 0.005f, 0.0075f, 0.8f);
                break;
            default:
                std::cout << "Invalid scene number! No change was done.";
                break;
        }
    }
}


#endif
