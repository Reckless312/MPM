cmake_minimum_required(VERSION 4.1)
set(CMAKE_CUDA_HOST_COMPILER /usr/bin/gcc-15)
project(MPM_Method LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)

set(SOURCE_FILES src/Libraries/glad.c src/main.cpp
        src/OpenGL/Shaders/Shader.cpp
        src/OpenGL/Shaders/Shader.h
        src/OpenGL/Program.cpp
        src/OpenGL/Program.h
        src/Exceptions/MPMException.cpp
        src/Exceptions/MPMException.h
        "src/Libraries/stb_image.cpp"
        src/OpenGL/Shaders/TextureLoader.cpp
        src/OpenGL/Shaders/TextureLoader.h
        src/OpenGL/Scene/Camera.cpp
        src/OpenGL/Scene/Camera.h
        src/Exceptions/Error.h
        src/OpenGL/Render/RenderObject.h
        src/OpenGL/Render/Mesh.cpp
        src/OpenGL/Render/Mesh.h
        src/OpenGL/Render/Model.cpp
        src/OpenGL/Render/Model.h
        src/OpenGL/Render/Particle.h
        src/OpenGL/Render/Particle.cpp
        src/CUDA/Structures/HashTable.h
        src/CUDA/Structures/HashTable.cu
        src/CUDA/Structures/GridBuffer.h
        src/CUDA/Structures/ParticleBuffer.h
        src/CUDA/Structures/Morton.h
        src/CUDA/Structures/Morton.cu
        src/CUDA/Preparation/RebuildMapping.h
        src/CUDA/Preparation/RebuildMapping.cu
        src/CUDA/Preparation/SortParticles.h
        src/CUDA/Preparation/SortParticles.cu
        src/CUDA/MPM/P2G.h
        src/CUDA/MPM/P2G.cu
        src/CUDA/MPM/UpdateGrid.h
        src/CUDA/MPM/UpdateGrid.cu
        src/CUDA/MPM/G2P.h
        src/CUDA/MPM/G2P.cu
        src/CUDA/Simulation.h
        src/CUDA/Simulation.cu
        src/CUDA/MeshBoundary.h
        src/CUDA/MeshBoundary.cpp
        src/CUDA/CudaCheck.h
        src/OpenGL/Simulation/Snowball.cpp
        src/OpenGL/Simulation/Snowball.h
        src/OpenGL/Simulation/Snowfall.cpp
        src/OpenGL/Simulation/Snowfall.h
        src/OpenGL/Simulation/Configuration.h
)

add_executable(MPM_Method ${SOURCE_FILES})

set_target_properties(MPM_Method PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

target_compile_options(MPM_Method PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-use_fast_math>)

target_include_directories(MPM_Method PUBLIC src src/Libraries/include)

target_compile_definitions(MPM_Method PRIVATE
        ASSETS_PATH="${CMAKE_SOURCE_DIR}/src/Assets"
        SHADERS_PATH="${CMAKE_SOURCE_DIR}/src/OpenGL/Shaders/GLSL/"
)

target_link_libraries(MPM_Method GL GLU glfw X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor assimp)