find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

find_path(GLM_PATH, glm) # Header only

add_library(aquila-engine SHARED
    init_engine.cpp
    render_engine.cpp
    aquila_engine.cpp
    vk_initializers.cpp
    pipeline_builder.cpp
    aq_mesh.cpp
    aq_vertex.cpp
    aq_node.cpp
    aq_camera.cpp
)

target_compile_definitions(aquila-engine PRIVATE PROJECT_PATH="${PROJECT_SOURCE_DIR}")

target_include_directories(aquila-engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(aquila-engine # Header only so include, but don't link
    PUBLIC ${GLM_PATH}
)

target_link_libraries(aquila-engine
    PUBLIC SDL2::SDL2
    PRIVATE Vulkan::Vulkan
    PUBLIC vma
)
