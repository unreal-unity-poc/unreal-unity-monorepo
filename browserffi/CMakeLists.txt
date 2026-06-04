cmake_minimum_required(VERSION 3.21)

project(RustBrowserFfiRenderer LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT APPLE)
    message(FATAL_ERROR "browserffi currently implements the macOS WKWebView host. Add a WebView2/GTK-WebKit host for other platforms.")
endif()

add_executable(rust_browserffi_renderer MACOSX_BUNDLE
    src/main.mm
)

set_source_files_properties(src/main.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_include_directories(rust_browserffi_renderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rust-engine/include
)

target_compile_definitions(rust_browserffi_renderer PRIVATE
    RUST_ENGINE_LIBRARY_PATH="${CMAKE_CURRENT_SOURCE_DIR}/native/macos/librust_engine.dylib"
)

target_link_libraries(rust_browserffi_renderer PRIVATE
    "-framework Cocoa"
    "-framework WebKit"
)

