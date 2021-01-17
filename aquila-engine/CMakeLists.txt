cmake_minimum_required(VERSION 3.0.0)
project(aquila-engine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")

add_subdirectory(third-party)
add_subdirectory(src)


set(GLSL_VALIDATOR "glslangValidator")

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag.glsl"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert.glsl"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp.glsl"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME_WLE)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS ${SPIRV})
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(aquila-engine Shaders)

export(TARGETS aquila-engine vma imgui FILE ${CMAKE_SOURCE_DIR}/aquila-engine-config.cmake)
