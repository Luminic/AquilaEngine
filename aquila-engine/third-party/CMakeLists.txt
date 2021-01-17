find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vma" )

add_library(stb-image INTERFACE)
target_include_directories(stb-image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")


add_library(imgui SHARED)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

target_sources(imgui PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdl.cpp"
)

target_link_libraries(imgui
    PRIVATE Vulkan::Vulkan
    PRIVATE SDL2::SDL2
)

