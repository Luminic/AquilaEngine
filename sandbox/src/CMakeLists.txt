add_executable(sandbox
    gameplay_engine.cpp
    camera_controller.cpp
    main.cpp
)

target_include_directories(sandbox
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

find_package(SDL2 REQUIRED)
find_package(aquila-engine REQUIRED PATHS ${PROJECT_SOURCE_DIR}/../aquila-engine/)

target_link_libraries(sandbox
    PRIVATE aquila-engine
)