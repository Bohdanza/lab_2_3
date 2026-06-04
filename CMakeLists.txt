cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(lab_2_3 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND WIN32)
    # 1. Tell Clang to generate CodeView debug information
    add_compile_options(-gcodeview)

    # 2. Tell the Linker to create a Windows PDB file
    # Note: We use -Xlinker to pass the /DEBUG flag directly to the underlying linker
    add_link_options("-Xlinker" "/DEBUG")
endif()

# --- SFML via FetchContent (mirrors D:/monacco/Monaco) -----------------------
# Build SFML 3.0 from source so no system-wide install is required. Static
# libraries keep the output self-contained (no SFML DLLs to copy next to the
# exe). SFML 3.0 uses miniaudio (header-only) instead of OpenAL, so there is no
# extra runtime dependency.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static libraries")

# SFML 3.0 bundles FreeType, whose CMakeLists declares an ancient minimum that
# CMake 4.x refuses. This shim lets those legacy subprojects configure anyway.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Allow legacy deps to configure on CMake 4.x")

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.1
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(lab_2_3 ${SOURCE_FILES})

target_link_libraries(lab_2_3 PRIVATE SFML::Graphics)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND WIN32)
    target_compile_options(lab_2_3 PRIVATE
            $<$<CONFIG:Debug>:-gcodeview>
            $<$<CONFIG:Debug>:-fstandalone-debug>
        )
endif()

if(WIN32)
    # SFML::Main provides a WinMain that forwards to our standard main(), so the
    # entry point works even under the Windows (GUI) subsystem.
    target_link_libraries(lab_2_3 PRIVATE SFML::Main)
endif()
