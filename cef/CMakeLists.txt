cmake_minimum_required(VERSION 3.21)

project(RustCefRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/native/macos/librust_engine.dylib")
elseif(WIN32)
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/native/windows/rust_engine.dll")
else()
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/native/linux/librust_engine.so")
endif()

add_library(rust_cef_bridge STATIC
    src/rust_engine_bridge.cpp
)

target_include_directories(rust_cef_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

target_compile_definitions(rust_cef_bridge PUBLIC
    RUST_ENGINE_LIBRARY_PATH="${RUST_ENGINE_LIBRARY_PATH}"
)

if(NOT WIN32)
    target_link_libraries(rust_cef_bridge PUBLIC dl)
endif()

add_executable(rust_cef_bridge_smoke
    src/smoke_main.cpp
)

target_link_libraries(rust_cef_bridge_smoke PRIVATE rust_cef_bridge)

add_executable(rust_cef_bridge_perf
    src/perf_main.cpp
)

target_link_libraries(rust_cef_bridge_perf PRIVATE rust_cef_bridge)
