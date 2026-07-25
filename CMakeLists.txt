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

# Engine-independent shared sources (no CNA/Easy3D dependencies).
# Shared by the game target and the engine-agnostic test suite.
set(GE_SHARED_SOURCES
    src/GalaxyEggbert/Worlds/BinaryIO.cpp
    src/GalaxyEggbert/Worlds/BitPacking.cpp
    src/GalaxyEggbert/Worlds/Chunk.cpp
    src/GalaxyEggbert/Worlds/World.cpp
    src/GalaxyEggbert/MoveObjectRecord.cpp
    src/GalaxyEggbert/BigDecorRecord.cpp
)

# =============================================================================
# Direct CNA + Easy3D target — GalaxyEggbertCNA (long-term direction, see CLAUDE.md
# "Current Direction Lock")
#
# ON by default; a plain `cmake -S . -B build` configures the maintained
# playable game, including terrain, objects, gameplay, HUD, sound, menus,
# persistence, and the in-game 3D editor. See CURRENT.md for verified status
# and easy3d.md/plan.md for the original migration analysis and task history.
#
# Build with:
#   cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON
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

    # Prefer the build tree's normal incremental objects over a second copy
    # in ccache. On this workstation ccache occupied 3.7 GiB while only
    # 22.7% of cacheable calls were hits, so misses amplified SSD writes.
    # This is an unforced fresh-build default: callers can still opt in with
    # -DCNA_USE_CCACHE=ON, and an existing cache choice remains untouched.
    set(CNA_USE_CCACHE OFF CACHE BOOL
        "Use ccache for CNA and sharp-runtime builds when available")

    # Default graphics backend for GalaxyEggbertCNA -- EasyGL (2026-07-18,
    # user request: make EasyGL the strongest/default backend, while keeping
    # other backends buildable for the future). BEFORE add_subdirectory(CNA_HOME)
    # below runs CNA's own (unforced, so lower-priority) default
    # `set(... CACHE ...)`. A plain (non-FORCE) `set(... CACHE ...)` only
    # takes effect the FIRST time a cache variable is populated, so this
    # still respects an explicit `-DCNA_GRAPHICS_BACKEND=...` on the configure
    # command line or an existing build directory's cache -- it only changes
    # the *default* for a fresh configure. Previously defaulted to VULKAN on
    # native (2026-07-09, to compare the two backends) -- reverted after this
    # session found a real Vulkan-only bug (missing NDC Y-flip in `../cna`'s
    # SkinnedEffect shaders, plan.md `E3D-MIG-069`) that EasyGL never had,
    # confirming EasyGL is the more reliable backend for this project today.
    # Vulkan/other backends remain fully buildable via an explicit
    # `-DCNA_GRAPHICS_BACKEND=VULKAN` (etc.) override -- this is a default
    # change only, not a removal of multi-backend support.
    set(CNA_GRAPHICS_BACKEND "EASYGL" CACHE STRING
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

    # Compatibility shim (found 2026-07-17): when reusing a parent-provided
    # CNA target (our case, always), easy-3d's own CMakeLists only forwards
    # the CNA_BACKEND_* compile definition to its `easy3d` target inside its
    # "build CNA myself" branch -- reused-CNA consumers of CNA's own
    # GraphicsBackendType.hpp (e.g. easy3d's BillboardMeshRenderer.cpp/
    # CubeMeshRenderer.cpp) never got it. Previously silent/tolerated;
    # started hard-failing (`#error "no CNA_BACKEND_* compile definition
    # set"`) after a large unrelated `../cna` merge (`feature/graphics`)
    # made that header stricter. This is a sibling-repo (`../cna`/
    # `../easy-3d`) gap, not something to fix by editing those repos here --
    # defining the same macro ourselves for this directory scope (inherited
    # by `easy3d_dep`, added right below) is a harmless, idempotent
    # workaround regardless of whether upstream fixes it later.
    if(CNA_GRAPHICS_BACKEND STREQUAL "EASYGL")
        add_compile_definitions(CNA_BACKEND_EASYGL)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "VULKAN")
        add_compile_definitions(CNA_BACKEND_VULKAN)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "SDL_RENDERER")
        add_compile_definitions(CNA_BACKEND_SDL_RENDERER)
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
        src/GalaxyEggbertCNA/Game/GEQuadBatch.cpp
        src/GalaxyEggbertCNA/Game/GESaveData.cpp
        src/GalaxyEggbertCNA/Editor/GEWorldEditor.cpp
        src/GalaxyEggbertCNA/Editor/GEWorldEditorUpdate.cpp
        src/GalaxyEggbertCNA/Editor/GEVoxelRaycast.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorHighlightRenderer.cpp
        src/GalaxyEggbertCNA/Editor/GEEditCommandStack.cpp
        src/GalaxyEggbertCNA/Editor/GEBoxRegion.cpp
        src/GalaxyEggbertCNA/Editor/GEPaletteCategories.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPalette.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteLayout.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteInput.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteRenderer.cpp
        src/GalaxyEggbertCNA/Editor/GECustomWorldStorage.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorBrowserScreen.cpp
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

    # A Windows build is a directly runnable bundle, not merely an .exe that
    # works while the MinGW/SDL build environment remains on PATH. Keep the
    # GCC/C++ runtime in the executable and use CNA's own helpers for the
    # remaining thread and SDL DLLs. --allow-multiple-definition is the same
    # MinGW static-libstdc++ workaround CNA applies to its own CnaTests:
    # PE/COFF otherwise fails to coalesce std::type_info::operator==.
    if(MINGW)
        target_link_options(GalaxyEggbertCNA PRIVATE
            -static-libgcc -static-libstdc++ -Wl,--allow-multiple-definition)
        cna_copy_mingw_runtime(GalaxyEggbertCNA)
        cna_copy_sdl_runtime(GalaxyEggbertCNA)

        # CNA's generic helper looks for SDL3::SDL3 and its analogous aliases.
        # This repository's Windows prebuilt exposes import libraries through
        # package configuration without making those targets visible here.
        # SDL3_DIR is nevertheless the same resolved package prefix used for
        # linking, so use its bin/ directory as a safe, build-local fallback.
        if(SDL3_DIR)
            get_filename_component(_ge_windows_sdl_prefix "${SDL3_DIR}/../../.." ABSOLUTE)
            foreach(_ge_windows_sdl_dll IN ITEMS SDL3.dll SDL3_image.dll SDL3_mixer.dll)
                set(_ge_windows_sdl_dll_path
                    "${_ge_windows_sdl_prefix}/bin/${_ge_windows_sdl_dll}")
                if(EXISTS "${_ge_windows_sdl_dll_path}")
                add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_ge_windows_sdl_dll_path}"
                        $<TARGET_FILE_DIR:GalaxyEggbertCNA>
                    COMMENT "Copying ${_ge_windows_sdl_dll} next to GalaxyEggbertCNA"
                    VERBATIM)
                endif()
            endforeach()
        endif()
    endif()

    # -------------------------------------------------------------------
    # Runtime asset staging
    #
    # Runtime assets used by GalaxyEggbertCNA are source-tracked here. Stage
    # only changed files after a relink. The old mobile-eggbert Content/
    # copy rewrote ~462 MiB every time, including ~437 MiB of unused 4x
    # artwork; the game only reads this repository's ~22 MiB Content tree.
    # -------------------------------------------------------------------
    function(ge_stage_runtime_directory sourceDirectory destinationName)
        add_custom_command(TARGET GalaxyEggbertCNA POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                "-DGE_COPY_SOURCE=${sourceDirectory}"
                "-DGE_COPY_DESTINATION=$<TARGET_FILE_DIR:GalaxyEggbertCNA>/${destinationName}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyDirectoryIfDifferent.cmake"
            COMMENT "Updating ${destinationName}/ runtime assets (changed files only)"
            VERBATIM)
    endfunction()

    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Content")
        ge_stage_runtime_directory("${CMAKE_CURRENT_SOURCE_DIR}/Content" "Content")
    endif()

    # galaxy-eggbert's own hand-authored 3D worlds (plan.md E3D-MIG-058) —
    # committed to this repository and staged next to the executable.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/worlds3d")
        ge_stage_runtime_directory("${CMAKE_CURRENT_SOURCE_DIR}/worlds3d" "worlds3d")
    endif()

    # galaxy-eggbert's own 3D-specific textures (e.g. grass_top.png, icon 107 —
    # NEXT.md §8 task 3). Same changed-files-only staging as worlds3d/ above.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/textures3d")
        ge_stage_runtime_directory("${CMAKE_CURRENT_SOURCE_DIR}/textures3d" "textures3d")
    endif()

    # Third-person-mode 3D character model(s) (NEXT.md §3, 2026-07-09) --
    # loaded via CNA's ContentManager, whose RootDirectory is pointed at this
    # directory in LoadContent(). Currently just a temporary
    # placeholder (avatars3d/blupi_placeholder/, see its own README.md) until
    # a real 3D Blupi model exists. Same copy-next-to-binary pattern as
    # worlds3d//textures3d above.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/avatars3d")
        ge_stage_runtime_directory("${CMAKE_CURRENT_SOURCE_DIR}/avatars3d" "avatars3d")
    endif()

    # -------------------------------------------------------------------
    # Emscripten (web) build (2026-07-17, user-requested prototype-for-web-
    # publishing target). Every asset directory above is baked into the
    # .wasm's virtual filesystem at link time via --preload-file instead of
    # copied next to a native binary, since there is no "next to the
    # binary" on the web. Only `Content/icons`, `Content/backgrounds`, and
    # `Content/sounds` are preloaded from this repository's Content/ tree
    # (not the full ~460MB directory) -- confirmed via direct grep that
    # GalaxyEggbertCNA never reads `Content/icons4x/` or
    # `Content/backgrounds4x/` (the 4x-scale variants in mobile-eggbert,
    # ~437MB combined, are genuinely unused here), keeping the web download
    # to a reasonable size (~25MB of assets).
    # -------------------------------------------------------------------
    if(EMSCRIPTEN)
        set_target_properties(GalaxyEggbertCNA PROPERTIES SUFFIX ".html")

        # CMake doesn't parse --shell-file/--pre-js out of target_link_options
        # to discover them as link dependencies (they're just linker-flag
        # strings to it) -- without LINK_DEPENDS, editing either file alone
        # (no source touched) leaves `cmake --build` believing the target is
        # already up to date and it silently keeps serving the OLD shell/
        # pre-js baked into the previous .html/.js (found 2026-07-17: a
        # shell-cna.html fix didn't take effect until a source file was
        # touched to force a relink).
        set_target_properties(GalaxyEggbertCNA PROPERTIES
            LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/shell-cna.html;${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/pre.js")

        target_link_options(GalaxyEggbertCNA PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=268435456
            -sNO_EXIT_RUNTIME=1
            -sMIN_WEBGL_VERSION=2
            -sMAX_WEBGL_VERSION=2
            -sFULL_ES3=1
            -lidbfs.js
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/shell-cna.html"
            "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/pre.js"
        )

        foreach(_content_dir Content/icons Content/backgrounds Content/sounds)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_content_dir}")
                target_link_options(GalaxyEggbertCNA PRIVATE
                    "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/${_content_dir}@/${_content_dir}")
            else()
                message(WARNING
                    "GalaxyEggbertCNA (web): ${_content_dir}/ not found at "
                    "'${CMAKE_CURRENT_SOURCE_DIR}/${_content_dir}'. The web build will still link, but "
                    "will have no ${_content_dir}/ in its virtual filesystem.")
            endif()
        endforeach()

        foreach(_own_dir worlds3d textures3d avatars3d)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_own_dir}")
                target_link_options(GalaxyEggbertCNA PRIVATE
                    "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/${_own_dir}@/${_own_dir}")
            endif()
        endforeach()
    endif()

    message(STATUS "GalaxyEggbertCNA target enabled (maintained playable game).")

    # Scripted verification for GEInteractionSystem (2026-07-10, platform
    # lift patrol/crate push/pickup collection) against the real
    # worlds3d/world999.vwr sample world. Needs CNA linked (GESound touches
    # Microsoft::Xna::Framework::Audio types) but not easy3d/SDL3main --
    # GESound::LoadContent() is never called, so no live audio device is
    # exercised at runtime, only symbols resolved at link time. Run from the
    # repo root (relative default path "worlds3d/world999.vwr"), same
    # quirk as VerifyBlupiMovement.
    add_executable(VerifyInteractionSystem
        tools/VerifyInteractionSystem.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        src/GalaxyEggbertCNA/Game/GESound.cpp
        src/GalaxyEggbertCNA/Game/GEInteractionSystem.cpp
        src/GalaxyEggbertCNA/Game/GETrainingHints.cpp
        src/GalaxyEggbertCNA/Game/GEBlupiController.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
        src/GalaxyEggbertCNA/Game/GEHud.cpp
        ${GE_SHARED_SOURCES}
    )
    target_include_directories(VerifyInteractionSystem PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
    )
    # Web/Node test wiring: this console tool is only ever run under Node
    # (via ctest, never in a browser -- no .html/SUFFIX set below), so
    # NODERAWFS gives it direct access to the real host filesystem instead
    # of an empty MEMFS, matching native behavior for its relative
    # "worlds3d/world999.vwr" default path with no --preload-file/embed
    # list to maintain.
    if(EMSCRIPTEN)
        target_link_options(VerifyInteractionSystem PRIVATE -sNODERAWFS=1)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND _ge_cna_backend_target)
        target_link_libraries(VerifyInteractionSystem PRIVATE
            -Wl,--start-group CNA ${_ge_cna_backend_target} -Wl,--end-group
            SHARP_RUNTIME)
    else()
        target_link_libraries(VerifyInteractionSystem PRIVATE CNA SHARP_RUNTIME)
    endif()
    if(TARGET easy3d::easy3d)
        target_link_libraries(VerifyInteractionSystem PRIVATE easy3d::easy3d)
    else()
        target_link_libraries(VerifyInteractionSystem PRIVATE easy3d)
    endif()
    if(BUILD_TESTING)
        # WORKING_DIRECTORY = repo root: this tool's default world path
        # ("worlds3d/world999.vwr") is relative, same quirk noted above.
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
        src/GalaxyEggbertCNA/Game/GEQuadBatch.cpp
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

    # Scripted editor verification is split by responsibility so failures
    # identify the broken subsystem instead of disappearing inside one
    # monolithic executable. Compile their shared real-editor support once:
    # repeating it in all four executables created four identical object
    # trees and rewrote them after every common editor change.
    add_library(GEEditorVerifySupport OBJECT
        src/GalaxyEggbertCNA/Editor/GEWorldEditor.cpp
        src/GalaxyEggbertCNA/Editor/GEWorldEditorUpdate.cpp
        src/GalaxyEggbertCNA/Editor/GEVoxelRaycast.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorHighlightRenderer.cpp
        src/GalaxyEggbertCNA/Editor/GEEditCommandStack.cpp
        src/GalaxyEggbertCNA/Editor/GEBoxRegion.cpp
        src/GalaxyEggbertCNA/Editor/GEPaletteCategories.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPalette.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteLayout.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteInput.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorPaletteRenderer.cpp
        src/GalaxyEggbertCNA/Editor/GECustomWorldStorage.cpp
        src/GalaxyEggbertCNA/Editor/GEEditorBrowserScreen.cpp
        src/GalaxyEggbertCNA/Game/GEQuadBatch.cpp
        src/GalaxyEggbertCNA/Game/GETileAtlas.cpp
        src/GalaxyEggbertCNA/Game/GEHud.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
        ${GE_SHARED_SOURCES}
    )
    target_include_directories(GEEditorVerifySupport PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
    )
    target_link_libraries(GEEditorVerifySupport PRIVATE CNA SHARP_RUNTIME)
    if(TARGET easy3d::easy3d)
        target_link_libraries(GEEditorVerifySupport PRIVATE easy3d::easy3d)
    else()
        target_link_libraries(GEEditorVerifySupport PRIVATE easy3d)
    endif()

    function(ge_add_editor_verify target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
        )
        target_link_libraries(${target} PRIVATE GEEditorVerifySupport)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND _ge_cna_backend_target)
            target_link_libraries(${target} PRIVATE
                -Wl,--start-group CNA ${_ge_cna_backend_target} -Wl,--end-group
                SHARP_RUNTIME)
        else()
            target_link_libraries(${target} PRIVATE CNA SHARP_RUNTIME)
        endif()
        if(TARGET easy3d::easy3d)
            target_link_libraries(${target} PRIVATE easy3d::easy3d)
        else()
            target_link_libraries(${target} PRIVATE easy3d)
        endif()
        if(BUILD_TESTING)
            add_test(NAME ${target} COMMAND ${target})
        endif()
    endfunction()

    ge_add_editor_verify(VerifyGEWorldEditor tools/VerifyGEWorldEditor.cpp)
    ge_add_editor_verify(VerifyGEEditorPalette tools/VerifyGEEditorPalette.cpp)
    ge_add_editor_verify(VerifyGEEditorStorage tools/VerifyGEEditorStorage.cpp)
    ge_add_editor_verify(VerifyGEEditorObjects tools/VerifyGEEditorObjects.cpp)

    # -------------------------------------------------------------------
    # Linux runtime bundle (BUILD-010)
    #
    # The game resolves all of its assets relative to its working directory.
    # Install a self-contained directory layout and a launcher that enters
    # that directory, then let CPack emit a portable .tar.gz bundle.  The
    # bundled SDL libraries remove the build-machine-specific SDL RPATH; OS
    # libraries such as the graphics stack and FFmpeg intentionally remain
    # distribution dependencies rather than being copied into the archive.
    # -------------------------------------------------------------------
    if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
        option(GALAXY_EGGBERT_ENABLE_LINUX_BUNDLE
            "Enable CPack's GalaxyEggbertCNA Linux runtime bundle" ON)

        if(GALAXY_EGGBERT_ENABLE_LINUX_BUNDLE)
            set_target_properties(GalaxyEggbertCNA PROPERTIES
                INSTALL_RPATH "$ORIGIN/lib")

            configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/packaging/run-galaxy-eggbert.sh.in"
                "${CMAKE_CURRENT_BINARY_DIR}/run-galaxy-eggbert"
                @ONLY)

            install(TARGETS GalaxyEggbertCNA
                RUNTIME DESTINATION "."
                COMPONENT Runtime)
            install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/run-galaxy-eggbert"
                DESTINATION "."
                COMPONENT Runtime)
            install(DIRECTORY Content worlds worlds3d textures3d avatars3d
                DESTINATION "."
                USE_SOURCE_PERMISSIONS
                COMPONENT Runtime)
            install(FILES LICENSE DESTINATION "." COMPONENT Runtime)
            install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/README-linux-bundle.md"
                DESTINATION "."
                RENAME "README-Linux.md"
                COMPONENT Runtime)

            # The SDL prebuilt install used by CNA provides license files.
            # Preserve them in the distributed bundle whenever SDL3_DIR points
            # at that standard package-config location.
            if(SDL3_DIR)
                get_filename_component(_ge_sdl_prefix "${SDL3_DIR}/../../.." ABSOLUTE)
                # Copy the versioned shared libraries and their SONAME
                # symlinks. The executable's INSTALL_RPATH resolves these
                # as $ORIGIN/lib after the bundle is extracted.
                install(DIRECTORY "${_ge_sdl_prefix}/lib/"
                    DESTINATION "lib"
                    COMPONENT Runtime
                    FILES_MATCHING PATTERN "libSDL3*.so*")
                if(EXISTS "${_ge_sdl_prefix}/share/licenses")
                    install(DIRECTORY "${_ge_sdl_prefix}/share/licenses/"
                        DESTINATION "licenses/SDL"
                        COMPONENT Runtime)
                endif()
            endif()

            set(CPACK_GENERATOR "TGZ")
            set(CPACK_PACKAGE_NAME "GalaxyEggbertCNA")
            set(CPACK_PACKAGE_FILE_NAME "GalaxyEggbertCNA-linux-${CMAKE_SYSTEM_PROCESSOR}")
            set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
            set(CPACK_COMPONENTS_ALL Runtime)
            # CPack otherwise invokes the root install with no component,
            # which recursively installs library/test artifacts from CNA and
            # Easy3D as well. Package only this project's Runtime component.
            set(CPACK_INSTALL_CMAKE_PROJECTS
                "${CMAKE_BINARY_DIR};${CMAKE_PROJECT_NAME};Runtime;/")
            include(CPack)
        endif()
    endif()

endif() # GALAXY_EGGBERT_BUILD_CNA

# =============================================================================
# Tools (engine-independent — pure C++ data model, no graphics)
# =============================================================================

# GEObjectIcons.hpp/.cpp is engine-agnostic (only depends on Def/
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

# Web/Node test wiring, same rationale as VerifyInteractionSystem above:
# Node-only console tool, NODERAWFS gives it real host filesystem access
# for its relative "worlds3d/world999.vwr" default path.
if(EMSCRIPTEN)
    target_link_options(VerifyBlupiMovement PRIVATE -sNODERAWFS=1)
endif()

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

# Web/Node test wiring: unlike the NODERAWFS tools above, GESaveData's own
# kSavePath is "/save/savedata.txt" under __EMSCRIPTEN__ (an absolute IDBFS
# mount point the real app's pre.js sets up for browser persistence, see
# GESaveData.hpp) -- NODERAWFS would instead map that absolute path onto the
# real host's own "/save" directory, which is wrong here. Reuses a minimal
# pre-js that just creates "/save" in this tool's own in-memory MEMFS (no
# IDBFS/persistence needed -- the test runs and exits within one process).
if(EMSCRIPTEN)
    set_target_properties(VerifyGESaveData PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/pre-test-save-dir.js")
    target_link_options(VerifyGESaveData PRIVATE
        -sFORCE_FILESYSTEM=1
        "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/cmake/web/pre-test-save-dir.js")
endif()

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

# INFRA-003 (plan.md §7): GEObjectIcons.hpp/.cpp is engine-agnostic (only
# depends on Def/ObjectType.hpp, see GenerateSampleWorld3D's own comment
# above) -- a data-integrity regression test for its 34 historical "Fixed"
# transcription bugs, same engine-independent precedent as
# VerifyTerrainAnimDivisor above.
add_executable(VerifyGetObjIcon
        tools/VerifyGetObjIcon.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
)

target_include_directories(VerifyGetObjIcon PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# INFRA-003 follow-up (plan.md §7): the independent programmatic
# reference-doc cross-check REMAKE-ANALYSIS.md's P0-2 actually asked for
# (VerifyGetObjIcon above is a regression lock, not this) -- parses
# mobile-eggbert-reference/08-animations.md's own §3.1/§3.2 tables at
# runtime and black-box-verifies GetObjIcon()'s actual cycle behavior
# against them. Same engine-independent precedent as VerifyGetObjIcon.
add_executable(VerifyObjIconAgainstReferenceDoc
        tools/VerifyObjIconAgainstReferenceDoc.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
)

target_include_directories(VerifyObjIconAgainstReferenceDoc PRIVATE
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

# INFRA-004 (plan.md §7): UnverifiedRenderMapping.hpp is a header-only data
# table (no CNA/graphics dependency), same engine-independent precedent as
# VerifyTileUvBounds above.
add_executable(VerifyUnverifiedRenderMapping
        tools/VerifyUnverifiedRenderMapping.cpp
)

target_include_directories(VerifyUnverifiedRenderMapping PRIVATE
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
# MoveObject: parsing (E3D-MIG-06x prerequisite) against real reference
# world files and the production CNA parser.
add_executable(VerifyMoveObjectTypesCna
        tools/VerifyMoveObjectTypesCna.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(VerifyMoveObjectTypesCna PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# Web/Node test wiring, same rationale as VerifyInteractionSystem above:
# Node-only console tool, NODERAWFS gives it real host filesystem access
# for its relative "../mobile-eggbert/worlds/*.txt" reads.
if(EMSCRIPTEN)
    target_link_options(VerifyMoveObjectTypesCna PRIVATE -sNODERAWFS=1)
endif()

# Same engine-agnostic-parser rationale as VerifyMoveObjectTypesCna above,
# for BigDecor: section parsing (NEXT.md §8 task 3) instead of MoveObject:.
add_executable(VerifyBigDecorParsingCna
        tools/VerifyBigDecorParsingCna.cpp
        src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp
        src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp
        ${GE_SHARED_SOURCES}
)

target_include_directories(VerifyBigDecorParsingCna PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyEggbertCNA
)

# Web/Node test wiring, same rationale as VerifyMoveObjectTypesCna above.
if(EMSCRIPTEN)
    target_link_options(VerifyBigDecorParsingCna PRIVATE -sNODERAWFS=1)
endif()

# plan.md TEST-002/008/009/010: registers these 4 engine-agnostic tools with
# ctest (VerifyInteractionSystem/VerifyGEInputPad are registered next to
# their own add_executable() above, inside the GALAXY_EGGBERT_BUILD_CNA
# block). VerifyBlupiMovement/VerifyMoveObjectTypesCna/VerifyBigDecorParsingCna
# all default to relative paths ("worlds3d/world999.vwr", "../mobile-eggbert/
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
    # No file/working-directory dependency -- GetObjIcon() is a pure function
    # of (ObjectType, phase) (plan.md INFRA-003).
    add_test(NAME VerifyGetObjIcon COMMAND VerifyGetObjIcon)
    # Reads mobile-eggbert-reference/08-animations.md, a repo-root-relative
    # path -- same WORKING_DIRECTORY reasoning as VerifyBlupiMovement above
    # (plan.md INFRA-003 follow-up).
    add_test(NAME VerifyObjIconAgainstReferenceDoc COMMAND VerifyObjIconAgainstReferenceDoc
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    # No file/working-directory dependency -- pure data-table integrity check
    # (plan.md INFRA-004).
    add_test(NAME VerifyUnverifiedRenderMapping COMMAND VerifyUnverifiedRenderMapping)
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
            tests/GalaxyEggbert/BigDecorRecordTests.cpp

            src/GalaxyEggbert/Worlds/BinaryIO.cpp
            src/GalaxyEggbert/Worlds/BitPacking.cpp
            src/GalaxyEggbert/Worlds/Chunk.cpp
            src/GalaxyEggbert/Worlds/World.cpp
            src/GalaxyEggbert/MoveObjectRecord.cpp
            src/GalaxyEggbert/BigDecorRecord.cpp
    )

    target_include_directories(GalaxyEggbertWorldsTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(GalaxyEggbertWorldsTests PRIVATE GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(GalaxyEggbertWorldsTests)
endif()
