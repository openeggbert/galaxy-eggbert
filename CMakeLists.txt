cmake_minimum_required(VERSION 3.21)
project(GalaxyEggbert LANGUAGES C CXX)

include(CTest)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if (NOT WIN32 AND CMAKE_BINARY_DIR MATCHES "build-windows")
    message(WARNING "Building in 'build-windows' directory, but WIN32 is not set. "
            "Make sure you used -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake "
            "on a CLEAN build directory.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Nova3D graphics backend selection
# -----------------------------------------------------------------------------
#
# Android and Emscripten use SDL_RENDERER only.
# Native desktop defaults to EASYGL.
#
# Override on desktop with:
#
#   cmake -S . -B build -DGALAXY_EGGBERT_GRAPHICS_BACKEND=SDL_RENDERER
#   cmake -S . -B build -DGALAXY_EGGBERT_GRAPHICS_BACKEND=EASYGL
#
# Supported values:
#   SDL_RENDERER
#   EASYGL
#   BGFX
#   VULKAN
#
# -----------------------------------------------------------------------------

set(GALAXY_EGGBERT_GRAPHICS_BACKEND "EASYGL" CACHE STRING "GalaxyEggbert graphics backend")
set_property(CACHE GALAXY_EGGBERT_GRAPHICS_BACKEND PROPERTY STRINGS
        SDL_RENDERER
        EASYGL
        EASY_GL
        BGFX
        VULKAN
)

string(TOUPPER "${GALAXY_EGGBERT_GRAPHICS_BACKEND}" GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER)

# Accept EASY_GL as an alias for EASYGL.
if(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "EASY_GL")
    set(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER "EASYGL")
endif()

# Android and Web: force SDL_RENDERER.
if(ANDROID OR EMSCRIPTEN)
    set(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER "SDL_RENDERER")
endif()

if(NOT GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER MATCHES "^(SDL_RENDERER|EASYGL|BGFX|VULKAN)$")
    message(FATAL_ERROR
            "Unsupported GALAXY_EGGBERT_GRAPHICS_BACKEND='${GALAXY_EGGBERT_GRAPHICS_BACKEND}'. "
            "Use SDL_RENDERER, EASYGL, BGFX, or VULKAN.")
endif()

message(STATUS "GalaxyEggbert graphics backend: ${GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER}")

# Configure CNA before add_subdirectory(../nova-3d Nova3D_dep).
if(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "SDL_RENDERER")
    set(CNA_BACKEND_SDL_RENDERER ON  CACHE BOOL "Enable SDL_Renderer backend in CNA" FORCE)
    set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "Enable easy-gl backend in CNA" FORCE)
    set(CNA_BACKEND_BGFX         OFF CACHE BOOL "Enable bgfx backend in CNA" FORCE)
    set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "Enable Vulkan backend in CNA" FORCE)
    set(CNA_GRAPHICS_BACKEND "SDL_RENDERER" CACHE STRING "Selected CNA backend" FORCE)

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "EASYGL")
    set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "Enable SDL_Renderer backend in CNA" FORCE)
    set(CNA_BACKEND_EASY_GL      ON  CACHE BOOL "Enable easy-gl backend in CNA" FORCE)
    set(CNA_BACKEND_BGFX         OFF CACHE BOOL "Enable bgfx backend in CNA" FORCE)
    set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "Enable Vulkan backend in CNA" FORCE)
    set(CNA_GRAPHICS_BACKEND "EASYGL" CACHE STRING "Selected CNA backend" FORCE)

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "BGFX")
    set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "Enable SDL_Renderer backend in CNA" FORCE)
    set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "Enable easy-gl backend in CNA" FORCE)
    set(CNA_BACKEND_BGFX         ON  CACHE BOOL "Enable bgfx backend in CNA" FORCE)
    set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "Enable Vulkan backend in CNA" FORCE)
    set(CNA_GRAPHICS_BACKEND "BGFX" CACHE STRING "Selected CNA backend" FORCE)

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "VULKAN")
    set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "Enable SDL_Renderer backend in CNA" FORCE)
    set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "Enable easy-gl backend in CNA" FORCE)
    set(CNA_BACKEND_BGFX         OFF CACHE BOOL "Enable bgfx backend in CNA" FORCE)
    set(CNA_BACKEND_VULKAN       ON  CACHE BOOL "Enable Vulkan backend in CNA" FORCE)
    set(CNA_GRAPHICS_BACKEND "VULKAN" CACHE STRING "Selected CNA backend" FORCE)
endif()
# -----------------------------------------------------------------------------
# Dependency configuration
# -----------------------------------------------------------------------------
#
# GalaxyEggbert uses Nova3D as its 3D engine (which internally uses CNA/easy-gl).
# Do not build examples/tests from vendored dependencies.
#
# Important:
# include(CTest) sets BUILD_TESTING=ON globally. Some subprojects use this
# variable directly, so temporarily disable it before add_subdirectory(../nova-3d).
# -----------------------------------------------------------------------------

set(GALAXY_EGGBERT_BUILD_TESTING "${BUILD_TESTING}" CACHE BOOL "Build GalaxyEggbert tests")

# Disable tests/examples for vendored dependencies.
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_TESTS   OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

# CNA options, if present.
set(CNA_BUILD_TESTS    OFF CACHE BOOL "Build CNA tests" FORCE)
set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "Build CNA examples" FORCE)

# Nova3D options.
set(NOVA3D_BUILD_TESTS    OFF CACHE BOOL "Build Nova3D tests" FORCE)
set(NOVA3D_BUILD_EXAMPLES OFF CACHE BOOL "Build Nova3D examples" FORCE)

# easy-gl option name variants, depending on how the library currently names them.
set(EASY_GL_BUILD_TESTS     OFF CACHE BOOL "Build easy-gl tests" FORCE)
set(EASY_GL_BUILD_EXAMPLES  OFF CACHE BOOL "Build easy-gl examples" FORCE)
set(EASYGL_BUILD_TESTS      OFF CACHE BOOL "Build easy-gl tests" FORCE)
set(EASYGL_BUILD_EXAMPLES   OFF CACHE BOOL "Build easy-gl examples" FORCE)
set(EASY_GL_ENABLE_TESTS    OFF CACHE BOOL "Enable easy-gl tests" FORCE)
set(EASY_GL_ENABLE_EXAMPLES OFF CACHE BOOL "Enable easy-gl examples" FORCE)
set(EASYGL_ENABLE_TESTS     OFF CACHE BOOL "Enable easy-gl tests" FORCE)
set(EASYGL_ENABLE_EXAMPLES  OFF CACHE BOOL "Enable easy-gl examples" FORCE)

add_subdirectory(../nova-3d Nova3D_dep)

# Restore GalaxyEggbert's own BUILD_TESTING value after vendored dependencies.
set(BUILD_TESTING "${GALAXY_EGGBERT_BUILD_TESTING}" CACHE BOOL "Build GalaxyEggbert tests" FORCE)

message(STATUS "CNA_BACKEND_SDL_RENDERER=${CNA_BACKEND_SDL_RENDERER}")
message(STATUS "CNA_BACKEND_EASY_GL=${CNA_BACKEND_EASY_GL}")
message(STATUS "CNA_BACKEND_BGFX=${CNA_BACKEND_BGFX}")
message(STATUS "CNA_BACKEND_VULKAN=${CNA_BACKEND_VULKAN}")
message(STATUS "CNA_GRAPHICS_BACKEND=${CNA_GRAPHICS_BACKEND}")

if(TARGET cna_backend_graphics_easygl)
    message(STATUS "CNA target exists: cna_backend_graphics_easygl")
else()
    message(STATUS "CNA target does not exist: cna_backend_graphics_easygl")
endif()

if(TARGET cna_backend_graphics_sdl_renderer)
    message(STATUS "CNA target exists: cna_backend_graphics_sdl_renderer")
else()
    message(STATUS "CNA target does not exist: cna_backend_graphics_sdl_renderer")
endif()

if(TARGET cna_backend_graphics_bgfx)
    message(STATUS "CNA target exists: cna_backend_graphics_bgfx")
else()
    message(STATUS "CNA target does not exist: cna_backend_graphics_bgfx")
endif()

if(TARGET cna_backend_graphics_vulkan)
    message(STATUS "CNA target exists: cna_backend_graphics_vulkan")
else()
    message(STATUS "CNA target does not exist: cna_backend_graphics_vulkan")
endif()

# Old global include kept for compatibility with the existing project layout.
# Prefer target_include_directories for new code.
include_directories(
        ../nova-3d/include
)

file(GLOB_RECURSE SOURCES
        "src/GalaxyEggbert/*.cpp"
        "src/GalaxyEggbert/Worlds/*.cpp"
)

# On Android the "executable" is a shared library loaded by SDLActivity.
# SDL's Java glue expects the native library to be named "libmain.so".
# We use a CMake variable so all subsequent target_* calls use the right name.
if(ANDROID)
    set(_game_target main)
    add_library(${_game_target} SHARED ${SOURCES})
else()
    set(_game_target GalaxyEggbert)
    add_executable(${_game_target}
            ${SOURCES}
            include/GalaxyEggbert/Worlds/BlockMetadata.hpp
    )
endif()

# On Emscripten the output is HTML+JS+WASM; set the suffix accordingly.
if(EMSCRIPTEN)
    set_target_properties(${_game_target} PROPERTIES SUFFIX ".html")
endif()

target_include_directories(${_game_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# SDL3 is provided by CNA via vendored submodules
# or system SDL if CNA_USE_SYSTEM_SDL=ON.
# No separate find_package(SDL3) call is needed here.

# -----------------------------------------------------------------------------
# Select concrete CNA backend target
# -----------------------------------------------------------------------------

set(_cna_backend_target "")

if(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "EASYGL")
    if(TARGET cna_backend_graphics_easygl)
        set(_cna_backend_target cna_backend_graphics_easygl)
    else()
        message(FATAL_ERROR
                "Requested EASYGL backend, but target cna_backend_graphics_easygl does not exist. "
                "Check CNA_BACKEND_EASY_GL and Nova3D/CNA's CMakeLists.txt.")
    endif()

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "SDL_RENDERER")
    if(TARGET cna_backend_graphics_sdl_renderer)
        set(_cna_backend_target cna_backend_graphics_sdl_renderer)
    else()
        message(STATUS
                "Requested SDL_RENDERER backend, but target cna_backend_graphics_sdl_renderer does not exist. "
                "Assuming it is linked through Nova3D/CNA directly.")
    endif()

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "BGFX")
    if(TARGET cna_backend_graphics_bgfx)
        set(_cna_backend_target cna_backend_graphics_bgfx)
    else()
        message(FATAL_ERROR
                "Requested BGFX backend, but target cna_backend_graphics_bgfx does not exist.")
    endif()

elseif(GALAXY_EGGBERT_GRAPHICS_BACKEND_UPPER STREQUAL "VULKAN")
    if(TARGET cna_backend_graphics_vulkan)
        set(_cna_backend_target cna_backend_graphics_vulkan)
    else()
        message(FATAL_ERROR
                "Requested VULKAN backend, but target cna_backend_graphics_vulkan does not exist.")
    endif()
endif()

message(STATUS "Selected CNA backend target: ${_cna_backend_target}")

# -----------------------------------------------------------------------------
# Link
# -----------------------------------------------------------------------------
#
# CNA and the selected graphics backend static library may have circular symbol
# references on Linux:
#
#   backend uses Color/Rectangle/Matrix from CNA
#   CNA uses backend interfaces
#
# GNU ld may need --start-group/--end-group.
# Emscripten and Android should not use this linker group.
# -----------------------------------------------------------------------------

if(EMSCRIPTEN)
    target_link_libraries(${_game_target} PRIVATE
            Nova3D
    )

elseif(ANDROID)
    # Android uses lld. The target is a shared library libmain.so loaded by SDLActivity.
    target_link_libraries(${_game_target} PRIVATE
            Nova3D
            android
            log
    )

else()
    target_link_libraries(${_game_target} PRIVATE
            Nova3D
    )
endif()

# -----------------------------------------------------------------------------
# Assets / platform-specific runtime configuration
# -----------------------------------------------------------------------------

if(ANDROID)
    # Android: no content copy needed.
    # Assets are expected to be packaged into the APK.

elseif(EMSCRIPTEN)
    # Emscripten link options.
    #
    # Asset virtual-filesystem layout:
    #
    #   ${CMAKE_CURRENT_SOURCE_DIR}/Content/backgrounds -> /Content/backgrounds
    #   ${CMAKE_CURRENT_SOURCE_DIR}/Content/icons       -> /Content/icons
    #   ${CMAKE_CURRENT_SOURCE_DIR}/Content/sounds      -> /Content/sounds
    #   ${CMAKE_CURRENT_SOURCE_DIR}/worlds              -> /worlds
    #
    # Save data is persisted via IDBFS mounted at /save.
    target_link_options(${_game_target} PRIVATE
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

else()
    # Native desktop: copy Content directory next to the executable.
    set(GALAXY_EGGBERT_CONTENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Content")

    if(EXISTS "${GALAXY_EGGBERT_CONTENT_DIR}")
        add_custom_command(TARGET ${_game_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${GALAXY_EGGBERT_CONTENT_DIR}"
                "$<TARGET_FILE_DIR:${_game_target}>/Content"
                COMMENT "Copying game content next to executable"
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Windows / MinGW runtime copying
# -----------------------------------------------------------------------------

if(WIN32)
    cna_copy_sdl_runtime(${_game_target})
endif()

if(MINGW)
    # Statically link the MinGW GCC and C++ runtime so the exe runs outside
    # CLion/MSYS2 without needing libgcc_s_seh-1.dll / libstdc++-6.dll.
    target_link_options(${_game_target} PRIVATE
            -static-libgcc
            -static-libstdc++
    )

    # libwinpthread-1.dll cannot be statically linked this way; copy it instead.
    cna_copy_mingw_runtime(${_game_target})
endif()

# -----------------------------------------------------------------------------
# Doxygen
# -----------------------------------------------------------------------------

find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(GalaxyEggbertDocs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

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

    target_link_libraries(GalaxyEggbertWorldsTests PRIVATE
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(GalaxyEggbertWorldsTests)
endif()