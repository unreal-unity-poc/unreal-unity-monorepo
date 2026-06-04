cmake_minimum_required(VERSION 3.21)

project(RustWebView2Renderer LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "WebView2 is a Windows-only target.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rust_webview2_renderer WIN32
    src/main.cpp
    src/rust_engine_bridge.cpp
)

target_include_directories(rust_webview2_renderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

target_compile_definitions(rust_webview2_renderer PRIVATE
    RUST_ENGINE_LIBRARY_PATH="${CMAKE_CURRENT_SOURCE_DIR}/native/windows/rust_engine.dll"
)

