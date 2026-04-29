cmake_minimum_required(VERSION 4.1)
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
        src/CUDA/HashTable.h
        src/CUDA/HashTable.cu
        src/CUDA/GridBuffer.h
        src/CUDA/ParticleBuffer.h
        src/CUDA/Morton.h
        src/CUDA/Morton.cu
        src/CUDA/RebuildMapping.h
        src/CUDA/RebuildMapping.cu
        src/CUDA/SortParticles.h
        src/CUDA/SortParticles.cu
        src/CUDA/P2G.h
        src/CUDA/P2G.cu
)

add_executable(MPM_Method ${SOURCE_FILES})

set_target_properties(MPM_Method PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(MPM_Method PUBLIC src src/Libraries/include)

target_compile_definitions(MPM_Method PRIVATE
        ASSETS_PATH="${CMAKE_SOURCE_DIR}/src/Assets"
        SHADERS_PATH="${CMAKE_SOURCE_DIR}/src/OpenGL/Shaders/GLSL/"
)

target_link_libraries(MPM_Method GL GLU glfw X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor assimp)