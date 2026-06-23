cmake_minimum_required(VERSION 3.21)
project(GalaxyEggbert LANGUAGES C CXX)

include(CTest)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if(NOT WIN32 AND CMAKE_BINARY_DIR MATCHES "build-windows")
    message(WARNING "Building in 'build-windows' directory, but WIN32 is not set. "
            "Make sure you used -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake "
            "on a CLEAN build directory.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Engine backend selection
# =============================================================================
#
#   cmake -S . -B build-u3d    -DGALAXY_EGGBERT_ENGINE=U3D    (default)
#   cmake -S . -B build-nova3d -DGALAXY_EGGBERT_ENGINE=NOVA3D
#
# U3D    — u3d-community/U3D (stable, full Urho3D scene graph)
# NOVA3D — Robert Vokac's Nova3D fork (WIP; switch here when it matures)
# =============================================================================

set(GALAXY_EGGBERT_ENGINE "U3D" CACHE STRING "Engine backend: U3D or NOVA3D")
set_property(CACHE GALAXY_EGGBERT_ENGINE PROPERTY STRINGS U3D NOVA3D)
string(TOUPPER "${GALAXY_EGGBERT_ENGINE}" _engine)

if(NOT _engine MATCHES "^(U3D|NOVA3D)$")
    message(FATAL_ERROR "Unsupported GALAXY_EGGBERT_ENGINE='${GALAXY_EGGBERT_ENGINE}'. Use U3D or NOVA3D.")
endif()

message(STATUS "GalaxyEggbert engine: ${_engine}")

# =============================================================================
# NOVA3D — graphics backend + dependency
# =============================================================================

if(_engine STREQUAL "NOVA3D")

    # Android and Emscripten force SDL_RENDERER; desktop defaults to EASYGL.
    set(GALAXY_EGGBERT_GRAPHICS_BACKEND "EASYGL" CACHE STRING "Nova3D graphics backend")
    set_property(CACHE GALAXY_EGGBERT_GRAPHICS_BACKEND PROPERTY STRINGS
            SDL_RENDERER EASYGL BGFX VULKAN)

    string(TOUPPER "${GALAXY_EGGBERT_GRAPHICS_BACKEND}" _gfx)
    if(_gfx STREQUAL "EASY_GL")
        set(_gfx "EASYGL")
    endif()
    if(ANDROID OR EMSCRIPTEN)
        set(_gfx "SDL_RENDERER")
    endif()

    if(NOT _gfx MATCHES "^(SDL_RENDERER|EASYGL|BGFX|VULKAN)$")
        message(FATAL_ERROR "Unsupported GALAXY_EGGBERT_GRAPHICS_BACKEND='${GALAXY_EGGBERT_GRAPHICS_BACKEND}'.")
    endif()
    message(STATUS "Nova3D graphics backend: ${_gfx}")

    # Pass backend selection into CNA/Nova3D.
    set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "" FORCE)
    set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "" FORCE)
    set(CNA_BACKEND_BGFX         OFF CACHE BOOL "" FORCE)
    set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "" FORCE)

    if(_gfx STREQUAL "SDL_RENDERER")
        set(CNA_BACKEND_SDL_RENDERER ON  CACHE BOOL "" FORCE)
        set(CNA_GRAPHICS_BACKEND "SDL_RENDERER" CACHE STRING "" FORCE)
    elseif(_gfx STREQUAL "EASYGL")
        set(CNA_BACKEND_EASY_GL      ON  CACHE BOOL "" FORCE)
        set(CNA_GRAPHICS_BACKEND "EASYGL" CACHE STRING "" FORCE)
    elseif(_gfx STREQUAL "BGFX")
        set(CNA_BACKEND_BGFX         ON  CACHE BOOL "" FORCE)
        set(CNA_GRAPHICS_BACKEND "BGFX" CACHE STRING "" FORCE)
    elseif(_gfx STREQUAL "VULKAN")
        set(CNA_BACKEND_VULKAN       ON  CACHE BOOL "" FORCE)
        set(CNA_GRAPHICS_BACKEND "VULKAN" CACHE STRING "" FORCE)
    endif()

    # Disable tests/examples in all sub-deps.
    set(GALAXY_EGGBERT_BUILD_TESTING "${BUILD_TESTING}" CACHE BOOL "Build GalaxyEggbert tests")
    set(BUILD_TESTING  OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CNA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(NOVA3D_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(NOVA3D_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(EASY_GL_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(EASY_GL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(EASYGL_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
    set(EASYGL_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
    set(EASY_GL_ENABLE_TESTS    OFF CACHE BOOL "" FORCE)
    set(EASY_GL_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(EASYGL_ENABLE_TESTS     OFF CACHE BOOL "" FORCE)
    set(EASYGL_ENABLE_EXAMPLES  OFF CACHE BOOL "" FORCE)

    add_subdirectory(../nova-3d Nova3D_dep)

    set(BUILD_TESTING "${GALAXY_EGGBERT_BUILD_TESTING}" CACHE BOOL "" FORCE)

endif() # NOVA3D

# =============================================================================
# U3D — locate pre-built library + headers
# =============================================================================

if(_engine STREQUAL "U3D")

    if(ANDROID)
        message(FATAL_ERROR
            "U3D engine on Android is not yet supported. "
            "See NEXT.md for the roadmap.")
    endif()

    # Resolve URHO3D_HOME: cmake option > env var > platform default.
    set(U3D_HOME "" CACHE PATH "Path to U3D build dir (include/ lib/ bin/). Leave empty to use local mirror.")

    if(U3D_HOME)
        set(URHO3D_HOME "${U3D_HOME}")
    elseif(DEFINED ENV{URHO3D_HOME})
        set(URHO3D_HOME "$ENV{URHO3D_HOME}")
    elseif(EMSCRIPTEN)
        set(URHO3D_HOME "/rv/data/library/github.com/u3d-community/U3D/build-web")
    else()
        set(URHO3D_HOME "/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug")
    endif()

    message(STATUS "URHO3D_HOME = ${URHO3D_HOME}")

    # Add U3D CMake modules so find_package(Urho3D) works.
    set(_u3d_src "/rv/data/library/github.com/u3d-community/U3D")
    list(APPEND CMAKE_MODULE_PATH "${_u3d_src}/cmake/Modules")

    # U3D on x86_64 is always 64-bit; prevent FindUrho3D.cmake from trying -m32.
    if(NOT DEFINED URHO3D_64BIT)
        set(URHO3D_64BIT 1 CACHE INTERNAL "Force 64-bit ABI for U3D")
    endif()

    find_package(Urho3D REQUIRED)

    message(STATUS "U3D include dirs : ${URHO3D_INCLUDE_DIRS}")
    message(STATUS "U3D libraries    : ${URHO3D_LIBRARIES}")

endif() # U3D

# =============================================================================
# Sources
# =============================================================================

file(GLOB_RECURSE SOURCES
        "src/GalaxyEggbert/*.cpp"
        "src/GalaxyEggbert/Worlds/*.cpp"
)

# =============================================================================
# Target definition  (executable on desktop/web, shared lib on Android)
# =============================================================================

if(ANDROID)
    # Android shared lib — name must be "main" for SDLActivity.
    set(_target main)
    add_library(${_target} SHARED ${SOURCES})
else()
    set(_target GalaxyEggbert)
    add_executable(${_target}
            ${SOURCES}
            include/GalaxyEggbert/Worlds/BlockMetadata.hpp
    )
endif()

if(EMSCRIPTEN)
    set_target_properties(${_target} PROPERTIES SUFFIX ".html")
endif()

# Game-internal headers (Worlds API, etc.)
target_include_directories(${_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbert
)

# =============================================================================
# Engine-specific: compile definitions, includes, link
# =============================================================================

if(_engine STREQUAL "U3D")

    target_include_directories(${_target} PRIVATE
            ${URHO3D_INCLUDE_DIRS}
    )

    target_link_libraries(${_target} PRIVATE
            ${URHO3D_LIBRARIES}
    )

    if(EMSCRIPTEN)
        # Emscripten: memory, filesystem, embind, shell, and content preloading.
        set(_u3d_desktop "/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug")
        set(_u3d_web    "/rv/data/library/github.com/u3d-community/U3D/build-web")
        target_link_libraries(${_target} PRIVATE embind)
        target_link_options(${_target} PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=134217728
            -sNO_EXIT_RUNTIME=1
            -sFORCE_FILESYSTEM=1
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/shell.html"
            "SHELL:--preload-file ${_u3d_desktop}/bin/Data@/Data"
            "SHELL:--preload-file ${_u3d_desktop}/bin/CoreData@/CoreData"
            "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content@/Content"
            "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/worlds@/worlds"
        )
    endif()

    # On Linux the static libUrho3D.a needs these system libs.
    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
        target_link_libraries(${_target} PRIVATE GL dl m rt pthread)
    endif()

    if(WIN32)
        target_link_libraries(${_target} PRIVATE
            opengl32 gdi32 winmm imm32 ole32 oleaut32 setupapi version uuid ws2_32 iphlpapi)
    endif()

    if(MINGW)
        target_link_options(${_target} PRIVATE -static-libgcc -static-libstdc++)
    endif()

    # Copy Data/ and CoreData/ next to the executable (desktop only).
    if(NOT ANDROID AND NOT EMSCRIPTEN)
        foreach(_dir Data CoreData)
            if(EXISTS "${URHO3D_HOME}/bin/${_dir}")
                add_custom_command(TARGET ${_target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                            "${URHO3D_HOME}/bin/${_dir}"
                            "$<TARGET_FILE_DIR:${_target}>/${_dir}"
                        COMMENT "Copying U3D ${_dir}/ next to executable"
                )
            endif()
        endforeach()

        # Copy game Content/ (tile textures, sprites, sounds) next to executable.
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Content")
            add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_CURRENT_SOURCE_DIR}/Content"
                        "$<TARGET_FILE_DIR:${_target}>/Content"
                    COMMENT "Copying game Content/ next to executable"
            )
        endif()

        # Copy worlds/ (level data) next to executable.
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/worlds")
            add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_CURRENT_SOURCE_DIR}/worlds"
                        "$<TARGET_FILE_DIR:${_target}>/worlds"
                    COMMENT "Copying worlds/ next to executable"
            )
        endif()
    endif()

elseif(_engine STREQUAL "NOVA3D")

    target_include_directories(${_target} PRIVATE
            ../nova-3d/include
    )

    # Resolve CNA backend target name.
    set(_cna "")
    if(_gfx STREQUAL "EASYGL" AND TARGET cna_backend_graphics_easygl)
        set(_cna cna_backend_graphics_easygl)
    elseif(_gfx STREQUAL "SDL_RENDERER" AND TARGET cna_backend_graphics_sdl_renderer)
        set(_cna cna_backend_graphics_sdl_renderer)
    elseif(_gfx STREQUAL "BGFX" AND TARGET cna_backend_graphics_bgfx)
        set(_cna cna_backend_graphics_bgfx)
    elseif(_gfx STREQUAL "VULKAN" AND TARGET cna_backend_graphics_vulkan)
        set(_cna cna_backend_graphics_vulkan)
    endif()

    if(EMSCRIPTEN)
        target_link_libraries(${_target} PRIVATE Nova3D)
    elseif(ANDROID)
        target_link_libraries(${_target} PRIVATE Nova3D android log)
    else()
        target_link_libraries(${_target} PRIVATE Nova3D)
    endif()

    # Web: embed content + save FS
    if(EMSCRIPTEN)
        target_link_options(${_target} PRIVATE
                -sALLOW_MEMORY_GROWTH=1
                -sSTACK_SIZE=1048576
                -sINITIAL_MEMORY=134217728
                -sFORCE_FILESYSTEM=1
                "-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap']"
                -lidbfs.js
                "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/pre.js"
                "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content/backgrounds@/Content/backgrounds"
                "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content/icons@/Content/icons"
                "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content/sounds@/Content/sounds"
                "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/worlds@/worlds"
        )
    endif()

    # Copy game content next to executable on desktop.
    if(NOT ANDROID AND NOT EMSCRIPTEN)
        set(_content "${CMAKE_CURRENT_SOURCE_DIR}/Content")
        if(EXISTS "${_content}")
            add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${_content}" "$<TARGET_FILE_DIR:${_target}>/Content"
                    COMMENT "Copying game content next to executable"
            )
        endif()
    endif()

    # Windows / MinGW runtime DLLs (CNA SDL3).
    if(WIN32)
        cna_copy_sdl_runtime(${_target})
    endif()
    if(MINGW)
        target_link_options(${_target} PRIVATE
                -static-libgcc -static-libstdc++)
        cna_copy_mingw_runtime(${_target})
    endif()

endif() # engine-specific

# =============================================================================
# Simple3D port target — GalaxyEggbertSimple3D
#
# Build with:
#   cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
#   cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
#
# The existing GalaxyEggbert (Urho3D) target is NOT replaced or modified.
# =============================================================================

option(GALAXY_EGGBERT_BUILD_SIMPLE3D "Build the Simple3D port target" OFF)

if(GALAXY_EGGBERT_BUILD_SIMPLE3D)

    # Locate simple-3d: explicit option > sibling ../simple-3d directory.
    set(SIMPLE3D_HOME "" CACHE PATH
        "Path to simple-3d source directory. Defaults to ../simple-3d.")

    if(NOT SIMPLE3D_HOME)
        set(SIMPLE3D_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../simple-3d")
    endif()

    if(NOT EXISTS "${SIMPLE3D_HOME}/CMakeLists.txt")
        message(FATAL_ERROR
            "simple-3d not found at '${SIMPLE3D_HOME}'. "
            "Clone it there or set -DSIMPLE3D_HOME=<path>.")
    endif()

    message(STATUS "simple-3d source: ${SIMPLE3D_HOME}")

    # Suppress tests/examples inside simple-3d when built as sub-dep.
    set(_s3d_save_BUILD_TESTING "${BUILD_TESTING}")
    set(BUILD_TESTING  OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory("${SIMPLE3D_HOME}" Simple3D_dep)

    set(BUILD_TESTING "${_s3d_save_BUILD_TESTING}" CACHE BOOL "" FORCE)

    # Engine-independent shared sources (no Urho3D dependencies).
    set(GE_SHARED_SOURCES
        src/GalaxyEggbert/Worlds/BinaryIO.cpp
        src/GalaxyEggbert/Worlds/BitPacking.cpp
        src/GalaxyEggbert/Worlds/Chunk.cpp
        src/GalaxyEggbert/Worlds/World.cpp
    )

    # New Simple3D-specific sources.
    set(GE_SIMPLE3D_SOURCES
        src/GalaxyEggbertSimple3D/Program.cpp
        src/GalaxyEggbertSimple3D/GalaxyEggbertSimpleGame.cpp
        src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp
        src/GalaxyEggbertSimple3D/Game/GETerrainRenderer.cpp
        src/GalaxyEggbertSimple3D/Game/GEBlupiController.cpp
        src/GalaxyEggbertSimple3D/Game/GEDecorSystem.cpp
        src/GalaxyEggbertSimple3D/Game/GEHud.cpp
        src/GalaxyEggbertSimple3D/Game/GECameraRig.cpp
    )

    add_executable(GalaxyEggbertSimple3D
        ${GE_SIMPLE3D_SOURCES}
        ${GE_SHARED_SOURCES}
    )

    target_include_directories(GalaxyEggbertSimple3D PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertSimple3D
        ${SIMPLE3D_HOME}/include
    )

    target_link_libraries(GalaxyEggbertSimple3D PRIVATE Simple3D)

    # Pass U3D binary path so the game can call SetResourcePrefixPaths().
    set(_u3d_desktop "/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug")
    target_compile_definitions(GalaxyEggbertSimple3D PRIVATE
        GALAXY_EGGBERT_U3D_BIN_PATH="${_u3d_desktop}/bin"
    )

    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
        target_link_libraries(GalaxyEggbertSimple3D PRIVATE GL dl m rt pthread)
    endif()

    if(MINGW)
        target_link_options(GalaxyEggbertSimple3D PRIVATE
            -static-libgcc -static-libstdc++)
    endif()

    # Copy U3D Data/ and CoreData/ next to the executable.
    foreach(_dir Data CoreData)
        if(EXISTS "${_u3d_desktop}/bin/${_dir}")
            add_custom_command(TARGET GalaxyEggbertSimple3D POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_u3d_desktop}/bin/${_dir}"
                    "$<TARGET_FILE_DIR:GalaxyEggbertSimple3D>/${_dir}"
                COMMENT "Copying U3D ${_dir}/ next to GalaxyEggbertSimple3D"
            )
        endif()
    endforeach()

    # Copy game Content/ and worlds/ next to executable.
    foreach(_d Content worlds)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_d}")
            add_custom_command(TARGET GalaxyEggbertSimple3D POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/${_d}"
                    "$<TARGET_FILE_DIR:GalaxyEggbertSimple3D>/${_d}"
                COMMENT "Copying ${_d}/ next to GalaxyEggbertSimple3D"
            )
        endif()
    endforeach()

    message(STATUS "GalaxyEggbertSimple3D target enabled.")

endif() # GALAXY_EGGBERT_BUILD_SIMPLE3D

# =============================================================================
# Doxygen
# =============================================================================

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(GalaxyEggbertDocs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
endif()

# =============================================================================
# Tests  (engine-independent — pure C++ data model, no graphics)
# =============================================================================

if(BUILD_TESTING AND NOT ANDROID AND NOT EMSCRIPTEN)
    include(FetchContent)

    if(NOT TARGET GTest::gtest_main)
        if(NOT TARGET gtest_main)
            FetchContent_Declare(
                    googletest
                    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            )
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
        endif()

        if(TARGET gtest AND NOT TARGET GTest::gtest)
            add_library(GTest::gtest ALIAS gtest)
        endif()
        if(TARGET gtest_main AND NOT TARGET GTest::gtest_main)
            add_library(GTest::gtest_main ALIAS gtest_main)
        endif()
    endif()

    add_executable(GalaxyEggbertWorldsTests
            tests/GalaxyEggbert/Worlds/BlockTests.cpp
            tests/GalaxyEggbert/Worlds/BitPackingTests.cpp
            tests/GalaxyEggbert/Worlds/ChunkTests.cpp
            tests/GalaxyEggbert/Worlds/WorldTests.cpp
            tests/GalaxyEggbert/Worlds/BlockMetadataTest.cpp

            src/GalaxyEggbert/Worlds/BinaryIO.cpp
            src/GalaxyEggbert/Worlds/BitPacking.cpp
            src/GalaxyEggbert/Worlds/Chunk.cpp
            src/GalaxyEggbert/Worlds/World.cpp
    )

    target_include_directories(GalaxyEggbertWorldsTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(GalaxyEggbertWorldsTests PRIVATE GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(GalaxyEggbertWorldsTests)
endif()
