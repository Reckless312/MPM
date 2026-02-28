cmake_minimum_required(VERSION 4.1)
project(MPM_Method LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)

set(SOURCE_FILES src/glad/glad.c src/main.cpp
        src/kernels/kernel.cu
        src/Shaders/Shader.cpp
        src/Shaders/Shader.h
        src/Program/Program.cpp
        src/Program/Program.h
        src/Exceptions/MPMException.cpp
        src/Exceptions/MPMException.h
        "src/stb image/stb_image.cpp"
        src/TextureLoader/TextureLoader.cpp
        src/TextureLoader/TextureLoader.h
        src/Camera/Camera.cpp
        src/Camera/Camera.h
        src/Exceptions/Error.h
)

add_executable(MPM_Method ${SOURCE_FILES})

set_target_properties(MPM_Method PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(MPM_Method PUBLIC src src/Libraries/include)

target_compile_definitions(MPM_Method PRIVATE
        ASSETS_PATH="${CMAKE_SOURCE_DIR}/src/Assets/"
        SHADERS_PATH="${CMAKE_SOURCE_DIR}/src/Shaders/"
)

target_link_libraries(MPM_Method GL GLU glfw X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor assimp)