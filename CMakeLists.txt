
cmake_minimum_required(VERSION 3.25)

project(SimpleRenderer
    VERSION 0.7.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW: window creation and input.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw)

# GLM: vectors, matrices and transformations.
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm)

# GLAD 2: modern OpenGL function loader.
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(glad)

if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
endif()

add_subdirectory(
    "${glad_SOURCE_DIR}/cmake"
    "${glad_BINARY_DIR}/cmake"
)

glad_add_library(glad_gl_core_33 STATIC
    API gl:core=3.3
)

# stb_image: image loading for PNG, JPEG and other formats.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(stb)

if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

find_package(OpenGL REQUIRED)

add_executable(SimpleRenderer
    src/main.cpp

    src/engine/Camera.cpp
    src/engine/Transform.cpp
    src/engine/GameObject.cpp
    src/engine/Scene.cpp
    src/engine/Mesh.cpp
    src/engine/ObjLoader.cpp
    src/engine/Texture.cpp
    src/engine/Material.cpp
    src/engine/Renderer.cpp
    src/engine/Shader.cpp
)

target_include_directories(SimpleRenderer PRIVATE
    "${stb_SOURCE_DIR}"
)

target_compile_definitions(SimpleRenderer PRIVATE
    SIMPLERENDERER_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(SimpleRenderer PRIVATE
    glfw
    glm::glm
    glad_gl_core_33
    OpenGL::GL
)

if(MSVC)
    target_compile_options(SimpleRenderer PRIVATE
        /W4
        /permissive-
    )
endif()

# Make relative asset paths resolve from the project directory.
set_property(
    TARGET SimpleRenderer
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "C:/SoftwareDevelopment/SimpleRenderer"
)
