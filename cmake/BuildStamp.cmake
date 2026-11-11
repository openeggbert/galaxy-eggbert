# The build's date and time for the main menu (the owner, 2026-11-11).
#
# ge_add_build_stamp(<target>) adds a generated source defining GalaxyEggbert::CNA::BuildStampText()
# (src/GalaxyEggbertCNA/BuildStamp.hpp). It is written again only when something <target> is
# linked from changed -- one of its sources, a header under include/ or src/, a library of its
# link closure, or its configuration (a fingerprint file(GENERATE) rewrites only when the build type,
# the flags or the target's definitions or options changed: such a change recompiles the game
# without touching any of the files above) -- that is, when <target> is linked again anyway. A build that changes nothing keeps
# the stamp and does not relink (the SSD rules of ../CLAUDE.md); __DATE__/__TIME__ in a plain
# source would instead keep the time that source was last compiled.

# The non-imported libraries <target> links, directly or through other targets; $<LINK_ONLY:x> is
# followed, any other generator expression is skipped (the stamp then misses only a change there).
function(_ge_link_closure_libraries target out_var)
    set(pending ${target})
    set(seen "")
    set(libraries "")
    while(pending)
        list(POP_FRONT pending current)
        if(current IN_LIST seen OR NOT TARGET "${current}")
            continue()
        endif()
        list(APPEND seen "${current}")
        get_target_property(aliased "${current}" ALIASED_TARGET)
        if(aliased)
            list(APPEND pending "${aliased}")
            continue()
        endif()
        get_target_property(imported "${current}" IMPORTED)
        if(imported)
            continue()
        endif()
        get_target_property(type "${current}" TYPE)
        if(type MATCHES "^(STATIC|SHARED|MODULE)_LIBRARY$" AND NOT current STREQUAL target)
            list(APPEND libraries "${current}")
        endif()
        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(linked "${current}" ${property})
            if(NOT linked)
                continue()
            endif()
            foreach(item IN LISTS linked)
                string(REGEX REPLACE "^\\$<LINK_ONLY:([^<>]+)>$" "\\1" item "${item}")
                if(NOT item MATCHES "\\$<" AND TARGET "${item}")
                    list(APPEND pending "${item}")
                endif()
            endforeach()
        endforeach()
    endwhile()
    set(${out_var} ${libraries} PARENT_SCOPE)
endfunction()

function(ge_add_build_stamp target)
    get_target_property(sources ${target} SOURCES)
    set(dependencies "")
    foreach(source IN LISTS sources)
        if(source MATCHES "\\$<")
            continue() # a generator expression: not a file to depend on
        endif()
        get_filename_component(source "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        list(APPEND dependencies "${source}")
    endforeach()
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
    _ge_link_closure_libraries(${target} libraries)

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/generated/src/GalaxyEggbertBuildStamp.cpp")
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
    set(configuration "${CMAKE_CURRENT_BINARY_DIR}/generated/src/GalaxyEggbertBuildStamp.configuration")
    file(GENERATE OUTPUT "${configuration}" CONTENT
"build type: ${CMAKE_BUILD_TYPE}
flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${build_type}} | ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${build_type}}
definitions: $<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>
options: $<TARGET_PROPERTY:${target},COMPILE_OPTIONS>
link options: $<TARGET_PROPERTY:${target},LINK_OPTIONS>
")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND "${CMAKE_COMMAND}" "-DOUT=${stamp}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteBuildStamp.cmake"
        DEPENDS ${dependencies} ${headers} ${libraries} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteBuildStamp.cmake"
                "${configuration}"
        COMMENT "Galaxy Eggbert: build stamp"
        VERBATIM)
    target_sources(${target} PRIVATE "${stamp}")
endfunction()
