add_executable(sandbox
    main.cpp
)

find_package(SDL2 REQUIRED)

find_package(aquila-engine REQUIRED PATHS ${PROJECT_SOURCE_DIR}/../aquila-engine/)

target_link_libraries(sandbox
    PRIVATE aquila-engine
)