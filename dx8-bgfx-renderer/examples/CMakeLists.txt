# Examples for dx8-bgfx-renderer

# Basic cube example
add_executable(basic_cube basic_cube.cpp)
target_link_libraries(basic_cube dx8-bgfx-renderer)
target_compile_definitions(basic_cube PRIVATE DX8BGFX_NO_MAIN=0)

# Multi-texture example
add_executable(multitexture multitexture.cpp)
target_link_libraries(multitexture dx8-bgfx-renderer)
target_compile_definitions(multitexture PRIVATE DX8BGFX_NO_MAIN=0)

# Set output directory
set_target_properties(basic_cube multitexture
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "Building examples: basic_cube, multitexture")
