find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

add_executable(vulkan-engine
    main.cpp
    # vk_engine.cpp
    vk_init_engine.cpp
    vk_render_engine.cpp
    vk_initializers.cpp
    pipeline_builder.cpp
    vk_mesh.cpp
)

target_compile_definitions(vulkan-engine PRIVATE PROJECT_PATH="${PROJECT_SOURCE_DIR}")

target_include_directories(vulkan-engine PRIVATE ${PROJECT_SOURCE_DIR}/include)

# target_include_directories(vulkan-engine PRIVATE ${PROJECT_SOURCE_DIR}/third-party/vma)
# target_link_libraries(vulkan-engine, )

target_link_libraries(vulkan-engine
    PRIVATE SDL2::SDL2
    PRIVATE Vulkan::Vulkan
    PUBLIC glm
    PRIVATE vma
)