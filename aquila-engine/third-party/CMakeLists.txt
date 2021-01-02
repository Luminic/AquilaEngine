find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vma" )

add_library(stb-image INTERFACE)
target_include_directories(stb-image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")
