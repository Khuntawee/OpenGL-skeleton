# Download LearnOpenGL utility headers automatically
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/learnopengl)

# Create a small root_directory.h used by filesystem.h so includes resolve.
file(WRITE ${CMAKE_BINARY_DIR}/learnopengl/root_directory.h "#pragma once\n// Defines the root path used by the FileSystem helper.\n// Set to the source directory so runtime looks at project resources.\nstatic const char* logl_root = \"${CMAKE_SOURCE_DIR}\";\n")
file(WRITE ${CMAKE_BINARY_DIR}/root_directory.h "#pragma once\n// Defines the root path used by the FileSystem helper.\n// Set to the source directory so runtime looks at project resources.\nstatic const char* logl_root = \"${CMAKE_SOURCE_DIR}\";\n")
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/camera.h
  ${CMAKE_BINARY_DIR}/learnopengl/camera.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/shader_m.h
  ${CMAKE_BINARY_DIR}/learnopengl/shader_m.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/filesystem.h
  ${CMAKE_BINARY_DIR}/learnopengl/filesystem.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/shader.h
  ${CMAKE_BINARY_DIR}/learnopengl/shader.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/model.h
  ${CMAKE_BINARY_DIR}/learnopengl/model.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/animation.h
  ${CMAKE_BINARY_DIR}/learnopengl/animation.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/animator.h
  ${CMAKE_BINARY_DIR}/learnopengl/animator.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/animdata.h
  ${CMAKE_BINARY_DIR}/learnopengl/animdata.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/assimp_glm_helpers.h
  ${CMAKE_BINARY_DIR}/learnopengl/assimp_glm_helpers.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/bone.h
  ${CMAKE_BINARY_DIR}/learnopengl/bone.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/mesh.h
  ${CMAKE_BINARY_DIR}/learnopengl/mesh.h
)
file(DOWNLOAD
  https://raw.githubusercontent.com/JoeyDeVries/LearnOpenGL/master/includes/learnopengl/model_animation.h
  ${CMAKE_BINARY_DIR}/learnopengl/model_animation.h
)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/learnopengl)
cmake_minimum_required(VERSION 3.16)
project(PlayableCharacter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Disable warnings as errors and suppress deprecated warnings for Assimp to fix vsprintf build errors on macOS
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLAD
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.35
)
FetchContent_MakeAvailable(glad)

# glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# stb (single header)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# assimp
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v5.2.5
)
FetchContent_MakeAvailable(assimp)

# include directories
include_directories(
  ${glad_SOURCE_DIR}/include
  ${glfw_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${assimp_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)


# Only main.cpp is kept locally, other utility files will be downloaded automatically
set(SRC src/main.cpp)
add_executable(PlayableCharacter ${SRC})

# Ensure target can find downloaded headers in the build directory
target_include_directories(PlayableCharacter PRIVATE
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/learnopengl
)

target_link_libraries(PlayableCharacter PRIVATE glfw glad assimp)

if (WIN32)
    target_link_libraries(PlayableCharacter PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(PlayableCharacter PRIVATE OpenGL::GL)
endif()

set_target_properties(PlayableCharacter PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Ensure resources are copied next to the produced binary so relative lookups (from the exe) succeed.
# This copies resources into the target's output directory (works with single- and multi-config generators).
add_custom_command(TARGET PlayableCharacter POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:PlayableCharacter>/assets
  COMMENT "Copying assets to the target binary directory"
)

# Copy shaders to the target's output directory
add_custom_command(TARGET PlayableCharacter POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:PlayableCharacter>/shaders
  COMMENT "Copying shaders to the target binary directory"
)

# Optional: install resources when running 'cmake --install'. Places resources next to the installed binary.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets
        DESTINATION .
        USE_SOURCE_PERMISSIONS)

# On Visual Studio, set LOGL_ROOT_PATH in the debugger environment so FileSystem::getPath() can use the source
# directory when running inside the VS debugger. This has effect only for the VS generator.
set_target_properties(PlayableCharacter PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "LOGL_ROOT_PATH=${CMAKE_SOURCE_DIR}"
)

message(STATUS "Runtime working directory set to: ${CMAKE_SOURCE_DIR}")
