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
# Simple3D port target — GalaxyEggbertSimple3D
#
# Build with:
#   cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
#   cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
# =============================================================================

option(GALAXY_EGGBERT_BUILD_SIMPLE3D "Build the Simple3D port target" ON)

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
        src/GalaxyEggbertSimple3D/Game/GESound.cpp
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

    set(_u3d_desktop "/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug")
    set(_u3d_web     "/rv/data/library/github.com/u3d-community/U3D/build-web")

    # On desktop pass U3D bin path for SetResourcePrefixPaths(); web uses preloaded VFS.
    if(NOT EMSCRIPTEN)
        target_compile_definitions(GalaxyEggbertSimple3D PRIVATE
            GALAXY_EGGBERT_U3D_BIN_PATH="${_u3d_desktop}/bin"
        )
    endif()

    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
        target_link_libraries(GalaxyEggbertSimple3D PRIVATE GL dl m rt pthread)
    endif()

    if(MINGW)
        target_link_options(GalaxyEggbertSimple3D PRIVATE
            -static-libgcc -static-libstdc++)
    endif()

    if(EMSCRIPTEN)
        set_target_properties(GalaxyEggbertSimple3D PROPERTIES SUFFIX ".html")
        target_link_libraries(GalaxyEggbertSimple3D PRIVATE embind)
        # Data/ and CoreData/ come from the desktop U3D build (same asset content).
        target_link_options(GalaxyEggbertSimple3D PRIVATE
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

    # Desktop only: copy U3D Data/ CoreData/ and game Content/ worlds/ next to executable.
    if(NOT EMSCRIPTEN AND NOT ANDROID)
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
    endif()

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
