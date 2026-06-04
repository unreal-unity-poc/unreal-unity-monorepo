cmake_minimum_required(VERSION 3.21)

project(RustV8BlinkRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rust_v8_blink_concept
    src/main.cpp
)

target_include_directories(rust_v8_blink_concept PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

