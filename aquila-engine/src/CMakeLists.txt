find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(assimp REQUIRED)

find_path(GLM_PATH, glm) # Header only

add_library(aquila-engine SHARED
    init_engine.cpp
    render_engine.cpp
    aquila_engine.cpp

    util/vk_initializers.cpp
    util/vk_types.cpp
    util/vk_utility.cpp
    util/vk_shaders.cpp
    util/vk_resizable_buffer.cpp
    util/vk_descriptor_set_builder.cpp
    util/aq_memory_manager.cpp
    util/pipeline_builder.cpp

    scene/aq_vertex.cpp
    scene/aq_mesh.cpp
    scene/aq_node.cpp
    scene/aq_model_loader.cpp
    scene/aq_camera.cpp

    scene/aq_texture.cpp
    scene/aq_material.cpp

    editor/aq_node_hierarchy_editor.cpp
    editor/file_browser.cpp
)

target_compile_definitions(aquila-engine 
    PRIVATE AQUILA_ENGINE_PATH="${PROJECT_SOURCE_DIR}"
    PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_include_directories(aquila-engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(aquila-engine # Header only so include but don't link
    PUBLIC ${GLM_PATH}
)

target_link_libraries(aquila-engine
    PUBLIC SDL2::SDL2
    PRIVATE Vulkan::Vulkan
    PRIVATE assimp
    PUBLIC vma
    PRIVATE stb-image
    PUBLIC imgui
)
