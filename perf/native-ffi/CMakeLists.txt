cmake_minimum_required(VERSION 3.21)

project(RustNativeFfiPerf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../target/debug/librust_engine.dylib")
elseif(WIN32)
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../target/debug/rust_engine.dll")
else()
    set(RUST_ENGINE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../target/debug/librust_engine.so")
endif()

add_executable(rust_native_ffi_perf
    src/main.cpp
)

target_include_directories(rust_native_ffi_perf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rust-engine/include
)

target_compile_definitions(rust_native_ffi_perf PRIVATE
    RUST_ENGINE_LIBRARY_PATH="${RUST_ENGINE_LIBRARY_PATH}"
)

if(NOT WIN32)
    target_link_libraries(rust_native_ffi_perf PRIVATE dl)
endif()

