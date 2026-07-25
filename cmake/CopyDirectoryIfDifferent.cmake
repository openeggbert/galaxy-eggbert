if(NOT DEFINED GE_COPY_SOURCE OR NOT DEFINED GE_COPY_DESTINATION)
    message(FATAL_ERROR
        "CopyDirectoryIfDifferent.cmake requires GE_COPY_SOURCE and GE_COPY_DESTINATION")
endif()

if(NOT IS_DIRECTORY "${GE_COPY_SOURCE}")
    message(FATAL_ERROR "Runtime asset source does not exist: ${GE_COPY_SOURCE}")
endif()

file(MAKE_DIRECTORY "${GE_COPY_DESTINATION}")

# `file(COPY)` still refreshes metadata on unchanged files. Traverse the
# small runtime tree and use CMake's byte-aware copy operation instead; a
# second stage then performs no data or metadata write for unchanged files.
file(GLOB_RECURSE GE_COPY_FILES
    LIST_DIRECTORIES false
    RELATIVE "${GE_COPY_SOURCE}"
    "${GE_COPY_SOURCE}/*")

foreach(GE_COPY_RELATIVE_PATH IN LISTS GE_COPY_FILES)
    set(GE_COPY_INPUT "${GE_COPY_SOURCE}/${GE_COPY_RELATIVE_PATH}")
    set(GE_COPY_OUTPUT "${GE_COPY_DESTINATION}/${GE_COPY_RELATIVE_PATH}")
    get_filename_component(GE_COPY_OUTPUT_DIR "${GE_COPY_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${GE_COPY_OUTPUT_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${GE_COPY_INPUT}" "${GE_COPY_OUTPUT}"
        RESULT_VARIABLE GE_COPY_RESULT)
    if(NOT GE_COPY_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to stage runtime asset: ${GE_COPY_INPUT}")
    endif()
endforeach()
