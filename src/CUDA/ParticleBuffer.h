#ifndef MPM_METHOD_PARTICLE_BUFFER_H
#define MPM_METHOD_PARTICLE_BUFFER_H

struct ParticleBlock {
    float positionX[32], positionY[32], positionZ[32];
    float velocityX[32], velocityY[32], velocityZ[32];
    float deformationGradient[9][32];
    float affineMomentumMatrix[9][32];
    float mass[32];
    float volume[32];
    float plasticVolume[32];
};

#endif
