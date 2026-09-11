# Ludelo — PlayStation Remote Play Client
cmake_minimum_required(VERSION 3.28)
project(Ludelo VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── vcpkg integration ────────────────────────────────────
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain")
endif()

# ─── Compiler flags ───────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /MP)
    add_compile_options(/EHsc)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    # Release optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

# ─── Find packages ────────────────────────────────────────
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# FFmpeg (find via pkg-config or vcpkg)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)
find_library(SWRESAMPLE_LIBRARY swresample)

# ImGui sources (vcpkg provides headers, we compile sources)
find_path(IMGUI_INCLUDE_DIR imgui.h)

# ─── Protobuf generation ──────────────────────────────────
file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/proto/*.proto")
if(PROTO_FILES)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
endif()

# ─── LudeloCore static library ────────────────────────────
file(GLOB_RECURSE CORE_SOURCES
    "src/PortalCore/*.cpp"
    "src/PortalCore/*.h"
)

add_library(LudeloCore STATIC
    ${CORE_SOURCES}
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)
add_library(PortalCore ALIAS LudeloCore)

target_include_directories(LudeloCore PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}"  # For generated protobuf headers
    "${AVCODEC_INCLUDE_DIR}"
    "${IMGUI_INCLUDE_DIR}"
)

target_link_libraries(LudeloCore PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    OpenSSL::SSL
    OpenSSL::Crypto
    Opus::opus
    protobuf::libprotobuf
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    CURL::libcurl
    cpr::cpr
    glm::glm
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
    # Windows system libraries
    ws2_32
    crypt32
    bcrypt
    hid
    setupapi
    d3d11
    dxgi
    windowsapp
)

target_compile_definitions(LudeloCore PUBLIC
    PORTAL_VERSION="${PROJECT_VERSION}"
    PORTAL_USER_AGENT="Ludelo/${PROJECT_VERSION}"
    VK_USE_PLATFORM_WIN32_KHR
)

# ─── UI library ───────────────────────────────────────────
file(GLOB_RECURSE UI_SOURCES
    "src/UI/*.cpp"
    "src/UI/*.h"
)

add_library(LudeloUI STATIC ${UI_SOURCES})
target_include_directories(LudeloUI PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(LudeloUI PUBLIC LudeloCore)

# ─── Platform library ────────────────────────────────────
file(GLOB_RECURSE PLATFORM_SOURCES
    "src/Platform/*.cpp"
    "src/Platform/*.h"
)

add_library(LudeloPlatform STATIC ${PLATFORM_SOURCES})
target_include_directories(LudeloPlatform PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(LudeloPlatform PUBLIC LudeloCore)

# ─── Main executable ─────────────────────────────────────
add_executable(Ludelo WIN32
    "src/main.cpp"
)

target_link_libraries(Ludelo PRIVATE
    LudeloCore
    LudeloUI
    LudeloPlatform
)

# Copy assets to build directory
add_custom_command(TARGET Ludelo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Ludelo>/assets"
)

# ─── Tests ────────────────────────────────────────────────
option(PORTAL_BUILD_TESTS "Build unit tests" ON)
if(PORTAL_BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES "tests/*.cpp")
    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SRC})
        target_link_libraries(${TEST_NAME} PRIVATE LudeloCore)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()

# ─── Install ──────────────────────────────────────────────
install(TARGETS Ludelo RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)

message(STATUS "")
message(STATUS "╔══════════════════════════════════════════╗")
message(STATUS "║  Ludelo v${PROJECT_VERSION}                           ║")
message(STATUS "║  PlayStation Remote Play Client          ║")
message(STATUS "║  Build Type: ${CMAKE_BUILD_TYPE}                    ║")
message(STATUS "╚══════════════════════════════════════════╝")
message(STATUS "")
