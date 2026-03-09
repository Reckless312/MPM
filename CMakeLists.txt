cmake_minimum_required(VERSION 4.1)
project(MPM_Method LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)

set(SOURCE_FILES src/Libraries/glad.c src/main.cpp
        src/CUDA/kernel.cu
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
        src/OpenGL/Model/Mesh.cpp
        src/OpenGL/Model/Mesh.h
        src/OpenGL/Model/Model.cpp
        src/OpenGL/Model/Model.h
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