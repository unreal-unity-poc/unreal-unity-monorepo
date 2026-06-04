cmake_minimum_required(VERSION 3.21)

project(RustQtRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(rust_qt_renderer
    src/main.cpp
)

qt_add_executable(rust_qt_perf
    src/perf_main.cpp
)

target_include_directories(rust_qt_renderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

target_include_directories(rust_qt_perf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

target_compile_definitions(rust_qt_renderer PRIVATE
    RUST_ENGINE_LIBRARY_PATH="${CMAKE_CURRENT_SOURCE_DIR}/native/macos/librust_engine.dylib"
)

target_compile_definitions(rust_qt_perf PRIVATE
    RUST_ENGINE_LIBRARY_PATH="${CMAKE_CURRENT_SOURCE_DIR}/native/macos/librust_engine.dylib"
)

target_link_libraries(rust_qt_renderer PRIVATE Qt6::Widgets)
target_link_libraries(rust_qt_perf PRIVATE Qt6::Widgets)
