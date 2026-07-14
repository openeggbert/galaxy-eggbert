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

# Engine-independent shared sources (no Urho3D/Simple3D/CNA dependencies).
# Shared by every game target — Simple3D, CNA, and the engine-agnostic test suite.
set(GE_SHARED_SOURCES
    src/GalaxyEggbert/Worlds/BinaryIO.cpp
    src/GalaxyEggbert/Worlds/BitPacking.cpp
    src/GalaxyEggbert/Worlds/Chunk.cpp
    src/GalaxyEggbert/Worlds/World.cpp
    src/GalaxyEggbert/MoveObjectRecord.cpp
)

# =============================================================================
# Simple3D port target — GalaxyEggbertSimple3D
#
# Historical reference only as of 2026-07-08 (see CLAUDE.md's "Current Direction Lock") — not
# built, fixed, or maintained going forward; its build is currently broken in this environment
# (missing/incompatible U3D prebuilt) and that is intentionally left unfixed. Default flipped to
# OFF 2026-07-09 so a plain `cmake -S . -B build` no longer tries to build it (found via a real
# bug: the Android Gradle build never overrides this option, so it was silently attempting to
# build this broken/frozen target instead of GalaxyEggbertCNA). The target definition itself is
# untouched — pass -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON explicitly if you deliberately want to try
# it (e.g. after fixing the U3D prebuilt yourself).
#
# Build with:
#   cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
#   cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
# =============================================================================

option(GALAXY_EGGBERT_BUILD_SIMPLE3D "Build the Simple3D port target (historical reference only, not maintained -- see CLAUDE.md)" OFF)

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
        src/GalaxyEggbertSimple3D/Game/GEExploSystem.cpp
        src/GalaxyEggbertSimple3D/Game/GEBridgeSystem.cpp
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

    # Scripted verification for the BigDecor: parsing fix (mobile-eggbert-2d-reference.md
    # §2.3) — depends on Simple3D for GEWorldRuntime's Vector3 fields, so it
    # only exists when the Simple3D target is being built.
    add_executable(VerifyBigDecorParsing
        tools/VerifyBigDecorParsing.cpp
        src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp
        ${GE_SHARED_SOURCES}
    )
    target_include_directories(VerifyBigDecorParsing PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertSimple3D
        ${SIMPLE3D_HOME}/include
    )
    target_link_libraries(VerifyBigDecorParsing PRIVATE Simple3D)

    # Scripted verification that the 12 previously-dropped MoveObject types
    # now spawn (mobile-eggbert-2d-reference.md §2.4).
    add_executable(VerifyMoveObjectTypes
        tools/VerifyMoveObjectTypes.cpp
        src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp
        ${GE_SHARED_SOURCES}
    )
    target_include_directories(VerifyMoveObjectTypes PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertSimple3D
        ${SIMPLE3D_HOME}/include
    )
    target_link_libraries(VerifyMoveObjectTypes PRIVATE Simple3D)

endif() # GALAXY_EGGBERT_BUILD_SIMPLE3D

# =============================================================================
# Direct CNA + Easy3D target — GalaxyEggbertCNA (long-term direction, see CLAUDE.md
# "Current Direction Lock")
#
# ON by default as of 2026-07-09 (flipped alongside GALAXY_EGGBERT_BUILD_SIMPLE3D's default
# flipping to OFF above, so a plain `cmake -S . -B build` builds something meaningful again) --
# this is now the actively developed target: renders real, textured, animated 3D terrain (all 4
# confirmed tile render modes + water), MoveObject/BigDecor billboards and platform-lift/crate
# objects, face-culled terrain. Still an invisible collision-only Blupi -- no visible Blupi
# character, HUD, or sound yet. See NEXT.md for current status (easy3d.md/plan.md "Direct CNA +
# Easy3D Migration" for the original analysis and task list).
#
# Build with:
#   cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
#   cmake --build build-cna --target GalaxyEggbertCNA
# =============================================================================

option(GALAXY_EGGBERT_BUILD_CNA "Build the direct CNA + Easy3D Galaxy Eggbert target (actively developed -- see CLAUDE.md)" ON)

if(GALAXY_EGGBERT_BUILD_CNA)

    # Locate CNA and easy-3d: explicit option > sibling directory.
    set(CNA_HOME "" CACHE PATH
        "Path to the CNA source directory. Defaults to ../cna.")
    if(NOT CNA_HOME)
        set(CNA_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../cna")
    endif()

    set(EASY3D_HOME "" CACHE PATH
        "Path to the easy-3d source directory. Defaults to ../easy-3d.")
    if(NOT EASY3D_HOME)
        set(EASY3D_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../easy-3d")
    endif()

    if(NOT EXISTS "${CNA_HOME}/CMakeLists.txt")
        message(FATAL_ERROR
            "CNA not found at '${CNA_HOME}'. Clone it there or set -DCNA_HOME=<path>.")
    endif()
    if(NOT EXISTS "${EASY3D_HOME}/CMakeLists.txt")
        message(FATAL_ERROR
            "easy-3d not found at '${EASY3D_HOME}'. Clone it there or set -DEASY3D_HOME=<path>.")
    endif()

    message(STATUS "CNA source: ${CNA_HOME}")
    message(STATUS "easy-3d source: ${EASY3D_HOME}")

    # Default graphics backend for GalaxyEggbertCNA (2026-07-09) -- CNA's own
    # CMakeLists.txt defaults CNA_GRAPHICS_BACKEND to EASYGL on Linux; this
    # sets it to VULKAN instead, BEFORE add_subdirectory(CNA_HOME) below runs
    # CNA's own (unforced, so lower-priority) default `set(... CACHE ...)`.
    # A plain (non-FORCE) `set(... CACHE ...)` only takes effect the FIRST
    # time a cache variable is populated, so this still respects an explicit
    # `-DCNA_GRAPHICS_BACKEND=...` on the configure command line or an
    # existing build directory's cache -- it only changes the *default* for
    # a fresh configure, per the user's ask (2026-07-09) to try Vulkan and
    # compare against the EasyGL-backend rendering investigated the same day.
    set(CNA_GRAPHICS_BACKEND "VULKAN" CACHE STRING
        "Graphics backend to use (SDL_RENDERER, EASYGL, BGFX, or VULKAN)")

    # Add CNA first: easy-3d's own CMakeLists.txt auto-detects an existing
    # `CNA` target from the parent project and links against it instead of
    # building its own copy (see easy-3d/CMakeLists.txt "Resolve CNA linkage").
    # Keep CNA's own tests/examples out of this build — we only need the library.
    if(NOT TARGET CNA)
        set(CNA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory("${CNA_HOME}" CNA_dep)
    endif()

    # Keep easy-3d's own tests/examples out of this build too.
    set(EASY3D_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(EASY3D_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET easy3d)
        add_subdirectory("${EASY3D_HOME}" easy3d_dep)
    endif()

    add_executable(GalaxyEggbertCNA
        src/GalaxyEggbertCNA/main.cpp
        src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        src/GalaxyEggbertCNA/Game/GETileAtlas.cpp
        src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp
        src/GalaxyEggbertCNA/Game/GETerrainAnimDivisor.cpp
        src/GalaxyEggbertCNA/Game/GEDirectionalCubeTiles.cpp
        src/GalaxyEggbertCNA/Game/GESwatchUv.cpp
        src/GalaxyEggbertCNA/Game/GEInnerPillarBoxTiles.cpp
        src/GalaxyEggbertCNA/Game/GEThinBarTiles.cpp
        src/GalaxyEggbertCNA/Game/GEInnerFlatPlateTiles.cpp
        src/GalaxyEggbertCNA/Game/GEPlateRotationMetadata.cpp
        src/GalaxyEggbertCNA/Game/GETripleCrossBillboardTiles.cpp
        src/GalaxyEggbertCNA/Game/GEBlupiController.cpp
        src/GalaxyEggbertCNA/Game/GECameraShake.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
        src/GalaxyEggbertCNA/Game/GESound.cpp
        src/GalaxyEggbertCNA/Game/GEInteractionSystem.cpp
        src/GalaxyEggbertCNA/Game/GEHud.cpp
        src/GalaxyEggbertCNA/Game/GETrainingHints.cpp
        src/GalaxyEggbertCNA/Game/GEInputPad.cpp
        src/GalaxyEggbertCNA/Game/GESaveData.cpp
        ${GE_SHARED_SOURCES}
    )

    target_include_directories(GalaxyEggbertCNA PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
    )

    # CNA's static library and its graphics-backend static library have
    # circular symbol references (mirrors how CNA's own examples/demo_2d
    # link themselves in ../cna/CMakeLists.txt) — GNU/Clang on non-Windows
    # need a linker group to resolve both directions; MSVC/MinGW do not.
    # CNA_GRAPHICS_BACKEND is the cache variable CNA itself set while
    # configuring above (defaults to EASYGL on Linux, SDL_RENDERER elsewhere;
    # see ../cna/CMakeLists.txt) — reuse it to name the matching backend target.
    set(_ge_cna_backend_target "")
    if(CNA_GRAPHICS_BACKEND STREQUAL "SDL_RENDERER")
        set(_ge_cna_backend_target "cna_backend_graphics_sdl_renderer")
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "EASYGL")
        set(_ge_cna_backend_target "cna_backend_graphics_easygl")
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "BGFX")
        set(_ge_cna_backend_target "cna_backend_graphics_bgfx")
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "VULKAN")
        set(_ge_cna_backend_target "cna_backend_graphics_vulkan")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND _ge_cna_backend_target)
        target_link_libraries(GalaxyEggbertCNA PRIVATE
            -Wl,--start-group CNA ${_ge_cna_backend_target} -Wl,--end-group
            SHARP_RUNTIME)
    else()
        target_link_libraries(GalaxyEggbertCNA PRIVATE CNA SHARP_RUNTIME)
    endif()

    # CNA_GamerServices (Microsoft::Xna::Framework::GamerServices/*.cpp,
    # includes AvatarRenderer's real-rendering extension) is a separate
    # static library gated behind CNA_ENABLE_NET (default ON) -- see CNA's
    # own CMakeLists.txt. Needed for the third-person placeholder model
    # (NEXT.md §3, 2026-07-09), whose code unconditionally references
    # AvatarRenderer/SkinnedModelEXT symbols (the std::filesystem::exists()
    # guard in LoadContent() is a runtime check on avatars3d/ being present,
    # not a way to avoid linking these symbols) -- if CNA_ENABLE_NET=OFF,
    # this target fails to link, it does not gracefully degrade. TARGET
    # check here just avoids a hard CMake configure-time error referencing a
    # nonexistent target; CNA_ENABLE_NET defaults ON so this is the normal
    # path in practice.
    if(TARGET CNA_GamerServices)
        target_link_libraries(GalaxyEggbertCNA PRIVATE CNA_GamerServices)
    endif()

    # Prefer the easy3d::easy3d alias if present.
    if(TARGET easy3d::easy3d)
        target_link_libraries(GalaxyEggbertCNA PRIVATE easy3d::easy3d)
    else()
        target_link_libraries(GalaxyEggbertCNA PRIVATE easy3d)
    endif()

    if(TARGET SDL3::SDL3main)
        target_link_libraries(GalaxyEggbertCNA PRIVATE SDL3::SDL3main)
    endif()

    # -------------------------------------------------------------------
    # Mobile Eggbert asset reuse (plan.md E3D-MIG-013/030 — decided strategy)
    #
    # mobile-eggbert is read-only: its Content/ and worlds/ directories are
    # copied next to the GalaxyEggbertCNA binary at build time, never
    # modified, and never committed into this repository (the destination
    # is inside the git-ignored build output directory). This makes the
    # sprite sheets, sounds, and world files available on disk for later
    # phases (terrain/Blupi rendering, sound, world loading) — nothing in
    # GalaxyEggbertCNA loads or reads them yet.
    # -------------------------------------------------------------------
    set(MOBILE_EGGBERT_HOME "" CACHE PATH
        "Path to the mobile-eggbert source directory (read-only asset source). Defaults to ../mobile-eggbert.")
    if(NOT MOBILE_EGGBERT_HOME)
        set(MOBILE_EGGBERT_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../mobile-eggbert")
    endif()

    if(NOT EMSCRIPTEN AND NOT ANDROID)
        foreach(_me_dir Content worlds)
            if(EXISTS "${MOBILE_EGGBERT_HOME}/${_me_dir}")
                add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${MOBILE_EGGBERT_HOME}/${_me_dir}"
                        "$<TARGET_FILE_DIR:GalaxyEggbertCNA>/${_me_dir}"
                    COMMENT "Copying mobile-eggbert ${_me_dir}/ next to GalaxyEggbertCNA (read-only source, not committed)"
                )
            else()
                message(WARNING
                    "GalaxyEggbertCNA: mobile-eggbert ${_me_dir}/ not found at "
                    "'${MOBILE_EGGBERT_HOME}/${_me_dir}'. GalaxyEggbertCNA will still build, but "
                    "will have no ${_me_dir}/ next to it. Set -DMOBILE_EGGBERT_HOME=<path> if "
                    "mobile-eggbert lives elsewhere.")
            endif()
        endforeach()
    endif()

    # galaxy-eggbert's own hand-authored 3D worlds (plan.md E3D-MIG-058) —
    # committed to this repository, copied next to the binary like Content/worlds above.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/worlds3d")
        add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/worlds3d"
                "$<TARGET_FILE_DIR:GalaxyEggbertCNA>/worlds3d"
            COMMENT "Copying worlds3d/ next to GalaxyEggbertCNA"
        )
    endif()

    # galaxy-eggbert's own 3D-specific textures (e.g. grass_top.png, icon 107 —
    # NEXT.md §8 task 3) — genuinely owned by galaxy-eggbert, NOT part of
    # mobile-eggbert's Content/ (which gets fully re-copied fresh from
    # mobile-eggbert on every build, so anything placed there would be wiped
    # out; see the "Copying mobile-eggbert Content/" step above). Same
    # copy-next-to-binary pattern as worlds3d/ above.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/textures3d")
        add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/textures3d"
                "$<TARGET_FILE_DIR:GalaxyEggbertCNA>/textures3d"
            COMMENT "Copying textures3d/ next to GalaxyEggbertCNA"
        )
    endif()

    # Third-person-mode 3D character model(s) (NEXT.md §3, 2026-07-09) --
    # loaded via CNA's ContentManager, whose RootDirectory is pointed at this
    # directory in LoadContent() (not "Content", to avoid the mobile-eggbert
    # Content/ wipe-and-recopy issue above). Currently just a temporary
    # placeholder (avatars3d/blupi_placeholder/, see its own README.md) until
    # a real 3D Blupi model exists. Same copy-next-to-binary pattern as
    # worlds3d//textures3d above.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/avatars3d")
        add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/avatars3d"
                "$<TARGET_FILE_DIR:GalaxyEggbertCNA>/avatars3d"
            COMMENT "Copying avatars3d/ next to GalaxyEggbertCNA"
        )
    endif()

    message(STATUS "GalaxyEggbertCNA target enabled (renders terrain, no gameplay yet).")

    # Scripted verification for GEInteractionSystem (2026-07-10, platform
    # lift patrol/crate push/pickup collection) against the real
    # worlds3d/world001.vwr sample world. Needs CNA linked (GESound touches
    # Microsoft::Xna::Framework::Audio types) but not easy3d/SDL3main --
    # GESound::LoadContent() is never called, so no live audio device is
    # exercised at runtime, only symbols resolved at link time. Run from the
    # repo root (relative default path "worlds3d/world001.vwr"), same
    # quirk as VerifyBlupiMovement.
    add_executable(VerifyInteractionSystem
        tools/VerifyInteractionSystem.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        src/GalaxyEggbertCNA/Game/GESound.cpp
        src/GalaxyEggbertCNA/Game/GEInteractionSystem.cpp
        src/GalaxyEggbertCNA/Game/GETrainingHints.cpp
        src/GalaxyEggbertCNA/Game/GEBlupiController.cpp
        ${GE_SHARED_SOURCES}
    )
    target_include_directories(VerifyInteractionSystem PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND _ge_cna_backend_target)
        target_link_libraries(VerifyInteractionSystem PRIVATE
            -Wl,--start-group CNA ${_ge_cna_backend_target} -Wl,--end-group
            SHARP_RUNTIME)
    else()
        target_link_libraries(VerifyInteractionSystem PRIVATE CNA SHARP_RUNTIME)
    endif()
    if(BUILD_TESTING)
        # WORKING_DIRECTORY = repo root: this tool's default world path
        # ("worlds3d/world001.vwr") is relative, same quirk noted above.
        add_test(NAME VerifyInteractionSystem COMMAND VerifyInteractionSystem
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    # Scripted verification for GEInputPad (2026-07-13, on-screen D-pad/
    # Jump/Action/Pause + Pause-row hit-testing) -- pure layout/press-state
    # math driven by synthetic MouseState values, no GraphicsDevice/window
    # needed at runtime, but GEInputPad.cpp's LoadContent()/DrawPlay()/
    # DrawPause() still reference CNA Texture2D/BasicEffect and
    # Easy3D::BillboardMeshRenderer symbols, so this links the same way
    # GalaxyEggbertCNA itself does (CNA + backend + SHARP_RUNTIME + easy3d).
    add_executable(VerifyGEInputPad
        tools/VerifyGEInputPad.cpp
        src/GalaxyEggbertCNA/Game/GEInputPad.cpp
    )
    target_include_directories(VerifyGEInputPad PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND _ge_cna_backend_target)
        target_link_libraries(VerifyGEInputPad PRIVATE
            -Wl,--start-group CNA ${_ge_cna_backend_target} -Wl,--end-group
            SHARP_RUNTIME)
    else()
        target_link_libraries(VerifyGEInputPad PRIVATE CNA SHARP_RUNTIME)
    endif()
    if(TARGET easy3d::easy3d)
        target_link_libraries(VerifyGEInputPad PRIVATE easy3d::easy3d)
    else()
        target_link_libraries(VerifyGEInputPad PRIVATE easy3d)
    endif()
    if(BUILD_TESTING)
        # No file/working-directory dependency -- LoadContent() is
        # deliberately never called (see this tool's own top comment).
        add_test(NAME VerifyGEInputPad COMMAND VerifyGEInputPad)
    endif()

endif() # GALAXY_EGGBERT_BUILD_CNA

# =============================================================================
# Tools (engine-independent — pure C++ data model, no graphics)
# =============================================================================

# GEObjectIcons.hpp/.cpp is engine-agnostic (only depends on def/
# ObjectType.hpp) despite living under src/GalaxyEggbertCNA/ -- linked here
# so the exhibition area can enumerate every ObjectType the renderer has an
# icon for (GetObjIcon), instead of duplicating that list by hand (same
# precedent as VerifyBlupiMovement linking GEBlupiController.cpp below).
add_executable(GenerateSampleWorld3D
        tools/GenerateSampleWorld3D.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
        src/GalaxyEggbertCNA/Game/GEPlateRotationMetadata.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(GenerateSampleWorld3D PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# GEBlupiController.hpp/.cpp are deliberately engine-agnostic (only depend on
# GalaxyEggbert::Worlds) despite living under src/GalaxyEggbertCNA/ — this
# tool links them without needing CNA at all (see plan.md E3D-MIG-060).
add_executable(VerifyBlupiMovement
        tools/VerifyBlupiMovement.cpp
        src/GalaxyEggbertCNA/Game/GEBlupiController.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(VerifyBlupiMovement PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# GESaveData.hpp/.cpp are deliberately engine-agnostic (only <string>/
# <fstream>) despite living under src/GalaxyEggbertCNA/ -- this tool links
# it without needing CNA at all, same precedent as VerifyBlupiMovement
# above.
add_executable(VerifyGESaveData
        tools/VerifyGESaveData.cpp
        src/GalaxyEggbertCNA/Game/GESaveData.cpp
)

target_include_directories(VerifyGESaveData PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# GETerrainAnimDivisor.hpp/.cpp (plan.md TEST-007, 2026-07-14) is a pure
# function of a BlockTypes icon id -- no CNA/graphics dependency at all,
# same engine-independent precedent as VerifyGESaveData/VerifyBlupiMovement
# above.
add_executable(VerifyTerrainAnimDivisor
        tools/VerifyTerrainAnimDivisor.cpp
        src/GalaxyEggbertCNA/Game/GETerrainAnimDivisor.cpp
)

target_include_directories(VerifyTerrainAnimDivisor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# BlockTypes::tileUV() (plan.md TEST-004) is a pure header-only function --
# no CNA/graphics dependency at all, same engine-independent precedent as
# VerifyGESaveData/VerifyBlupiMovement/VerifyTerrainAnimDivisor above.
add_executable(VerifyTileUvBounds
        tools/VerifyTileUvBounds.cpp
)

target_include_directories(VerifyTileUvBounds PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# GECameraShake.hpp/.cpp (plan.md CAM-008..013) is deliberately
# engine-agnostic (no CNA/graphics dependency) despite living under
# src/GalaxyEggbertCNA/ -- this tool links it without needing CNA at all,
# same precedent as VerifyGESaveData/VerifyTerrainAnimDivisor above.
add_executable(VerifyCameraShake
        tools/VerifyCameraShake.cpp
        src/GalaxyEggbertCNA/Game/GECameraShake.cpp
)

target_include_directories(VerifyCameraShake PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# GEWorldRuntime.hpp/.cpp (CNA) are deliberately engine-agnostic (only
# depend on GalaxyEggbert::Worlds/def, plain floats instead of a CNA/XNA
# Vector3) — this tool links it without needing CNA at all, verifying
# MoveObject: parsing (E3D-MIG-06x prerequisite) the same way
# VerifyMoveObjectTypes.cpp does for the Simple3D-side parser.
add_executable(VerifyMoveObjectTypesCna
        tools/VerifyMoveObjectTypesCna.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(VerifyMoveObjectTypesCna PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# Same engine-agnostic-parser rationale as VerifyMoveObjectTypesCna above,
# for BigDecor: section parsing (NEXT.md §8 task 3) instead of MoveObject:.
add_executable(VerifyBigDecorParsingCna
        tools/VerifyBigDecorParsingCna.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(VerifyBigDecorParsingCna PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# plan.md TEST-002/008/009/010: registers these 4 engine-agnostic tools with
# ctest (VerifyInteractionSystem/VerifyGEInputPad are registered next to
# their own add_executable() above, inside the GALAXY_EGGBERT_BUILD_CNA
# block). VerifyBlupiMovement/VerifyMoveObjectTypesCna/VerifyBigDecorParsingCna
# all default to relative paths ("worlds3d/world001.vwr", "../mobile-eggbert/
# worlds/*.txt") that only resolve from the repo root (NEXT.md §7's own
# note) -- WORKING_DIRECTORY makes that explicit instead of relying on
# whatever directory ctest happens to be invoked from. VerifyGESaveData
# needs no such override: it only touches a scratch "savedata.txt" in
# its own cwd and cleans it up itself, so the default build-dir cwd
# keeps the repo root clean.
if(BUILD_TESTING)
    add_test(NAME VerifyBlupiMovement COMMAND VerifyBlupiMovement
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_test(NAME VerifyGESaveData COMMAND VerifyGESaveData)
    add_test(NAME VerifyMoveObjectTypesCna COMMAND VerifyMoveObjectTypesCna
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_test(NAME VerifyBigDecorParsingCna COMMAND VerifyBigDecorParsingCna
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    # No file/working-directory dependency -- AnimDivisor() is a pure
    # function of a BlockTypes icon id (plan.md TEST-007).
    add_test(NAME VerifyTerrainAnimDivisor COMMAND VerifyTerrainAnimDivisor)
    # No file/working-directory dependency -- tileUV() is a pure header-only
    # function of a BlockTypes icon id (plan.md TEST-004).
    add_test(NAME VerifyTileUvBounds COMMAND VerifyTileUvBounds)
    # No file/working-directory dependency -- GECameraShake is a pure
    # tick-based state machine (plan.md CAM-008..013).
    add_test(NAME VerifyCameraShake COMMAND VerifyCameraShake)
endif()

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
            tests/GalaxyEggbert/MoveObjectRecordTests.cpp

            src/GalaxyEggbert/Worlds/BinaryIO.cpp
            src/GalaxyEggbert/Worlds/BitPacking.cpp
            src/GalaxyEggbert/Worlds/Chunk.cpp
            src/GalaxyEggbert/Worlds/World.cpp
            src/GalaxyEggbert/MoveObjectRecord.cpp
    )

    target_include_directories(GalaxyEggbertWorldsTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(GalaxyEggbertWorldsTests PRIVATE GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(GalaxyEggbertWorldsTests)
endif()
