cmake_minimum_required(VERSION 3.16)
project(dx8-bgfx-renderer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(DX8BGFX_BUILD_EXAMPLES "Build example applications" ON)
option(DX8BGFX_BUILD_TESTS "Build unit tests" OFF)

# Find bgfx
# You need to set BGFX_DIR to your bgfx installation
if(NOT BGFX_DIR)
    set(BGFX_DIR "${CMAKE_SOURCE_DIR}/../bgfx" CACHE PATH "Path to bgfx")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${BGFX_DIR}/include
    ${BGFX_DIR}/../bx/include
    ${BGFX_DIR}/../bimg/include
)

# Library sources
set(DX8BGFX_SOURCES
    src/core/dx8_types.cpp
    src/state/dx8_state_manager.cpp
    src/shader/dx8_shader_key.cpp
    src/shader/dx8_shader_generator.cpp
    src/shader/dx8_shader_cache.cpp
    src/core/dx8_renderer.cpp
    src/core/dx8_uniform_manager.cpp
)

set(DX8BGFX_HEADERS
    include/dx8bgfx/dx8_types.h
    include/dx8bgfx/dx8_constants.h
    include/dx8bgfx/dx8_state_manager.h
    include/dx8bgfx/dx8_shader_key.h
    include/dx8bgfx/dx8_shader_generator.h
    include/dx8bgfx/dx8_shader_cache.h
    include/dx8bgfx/dx8_renderer.h
    include/dx8bgfx/dx8_uniform_manager.h
)

# Create library
add_library(dx8-bgfx-renderer STATIC ${DX8BGFX_SOURCES} ${DX8BGFX_HEADERS})

target_include_directories(dx8-bgfx-renderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link bgfx libraries
if(WIN32)
    set(BGFX_LIBS bgfx bx bimg)
else()
    set(BGFX_LIBS bgfx bx bimg dl pthread)
endif()

target_link_libraries(dx8-bgfx-renderer PUBLIC ${BGFX_LIBS})

# Examples
if(DX8BGFX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(DX8BGFX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
install(TARGETS dx8-bgfx-renderer
    EXPORT dx8-bgfx-renderer-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/dx8bgfx
    DESTINATION include
)

# Shader compilation helper
function(compile_bgfx_shader SHADER_FILE SHADER_TYPE OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)

    set(SHADER_OUTPUT "${OUTPUT_DIR}/${SHADER_NAME}.bin")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${BGFX_DIR}/.build/linux64_gcc/bin/shadercRelease
            -f ${SHADER_FILE}
            -o ${SHADER_OUTPUT}
            --type ${SHADER_TYPE}
            --platform linux
            -p spirv
            -i ${BGFX_DIR}/src
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_NAME}"
    )
endfunction()
