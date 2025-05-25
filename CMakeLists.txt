cmake_minimum_required(VERSION 3.11) # FetchContent works best with 3.11+
project(duck LANGUAGES CXX C) # Added C language

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Dependencies ---
include(FetchContent)

# GLAD (OpenGL Loader)
# Assuming glad.c and glad.h are in the src/ directory or src/glad/
# If glad.h is in src/glad/, you might need:
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad)
set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src) # For main.cpp to find glad.h
set(GLAD_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)


# GLFW (Windowing and Input) - Fetched using FetchContent
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8 # Or a more recent stable tag. Check GLFW releases.
)

# Set GLFW CMake options BEFORE FetchContent_MakeAvailable
# These will be used when GLFW's CMakeLists.txt is processed.
# Use CACHE ... FORCE to ensure these settings override GLFW's defaults.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW examples" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target for GLFW" FORCE)
# Force GLFW to be built as a static library. GLFW's top-level CMakeLists.txt uses BUILD_SHARED_LIBS.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build GLFW as a shared library" FORCE)

FetchContent_MakeAvailable(glfw) # This populates and adds the subdirectory

# GLM (Mathematics Library) - Fetched using FetchContent
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8 # Or a more recent stable tag
)
FetchContent_MakeAvailable(glm)

# stb_image (Image Loading)
# Assuming stb_image.h is in the src/ directory.

# --- Project Sources ---
set(PROJECT_SOURCES
        src/main.cpp
        src/Shader.cpp
        src/Shader.h
        src/Camera.cpp
        src/Camera.h
        src/Model.h
        src/Model.cpp
        src/DuckAnimator.cpp
        src/DuckAnimator.h
        src/WaterSimulator.cpp
        src/WaterSimulator.h
        ${GLAD_SOURCE}
        src/stb_image.h
)

# --- Executable ---
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES}) # PROJECT_NAME is now "duck"

# --- Include Directories (Must be AFTER add_executable for the main target) ---
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # For Shader.h, Camera.h, stb_image.h
        ${GLAD_INCLUDE_DIR}             # For glad.h
        ${glm_SOURCE_DIR}               # For GLM headers (glm_INCLUDE_DIR could also be used)
        # GLFW headers should be available via the 'glfw' target link if glfw is linked correctly
        # For FetchContent, glfw target usually handles its own include dirs.
        # If glfw_INCLUDE_DIRS is populated by FetchContent_MakeAvailable, you could add it:
        # ${glfw_INCLUDE_DIRS}
)

# --- Linking (Must be AFTER add_executable for the main target) ---
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw # Link against the glfw target provided by FetchContent
)

# Platform-specific linking for OpenGL
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
elseif (UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    # GLFW on Linux often needs X11 libraries, pthread, dl, etc.
    # The 'glfw' target from FetchContent should handle these.
endif()


# --- Copy Shaders and Textures to Build Directory ---
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(TEXTURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/textures)

# Determine runtime output directory
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    if(CMAKE_CONFIGURATION_TYPES) # Multi-config generator (e.g. Visual Studio, Xcode)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    else() # Single-config generator (e.g. Makefiles, Ninja)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
endif()
message(STATUS "Runtime output directory set to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")


# Function to copy directory if it exists
function(copy_resource_directory SOURCE_DIR DEST_SUBDIR)
    if(EXISTS ${SOURCE_DIR})
        get_filename_component(DEST_NAME ${SOURCE_DIR} NAME)
        set(FINAL_DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${DEST_SUBDIR})

        add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD # PROJECT_NAME is now "duck"
                COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${SOURCE_DIR} ${FINAL_DEST_DIR}/${DEST_NAME}
                COMMENT "Copying ${DEST_NAME} to build output directory"
        )
        # For IDEs to see resources (copies at configure time)
        file(COPY ${SOURCE_DIR} DESTINATION ${FINAL_DEST_DIR})

    else()
        message(WARNING "Resource directory not found: ${SOURCE_DIR}")
    endif()
endfunction()

copy_resource_directory(${SHADER_DIR} "") # Copies 'shaders' directory into runtime output dir
copy_resource_directory(${TEXTURE_DIR} "") # Copies 'textures' directory into runtime output dir


message(STATUS "GLM source dir: ${glm_SOURCE_DIR}")
message(STATUS "GLFW source dir: ${glfw_SOURCE_DIR}")
message(STATUS "GLFW binary dir (build location for glfw): ${glfw_BINARY_DIR}")

