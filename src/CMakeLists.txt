find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(vulkan-engine
main.cpp
vk_engine.cpp
vk_initializers.cpp
)

target_include_directories(vulkan-engine PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(vulkan-engine
    PRIVATE SDL2::SDL2
    PRIVATE Vulkan::Vulkan
)
