cmake_minimum_required(VERSION 3.18)
project(CryptoTracker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find system packages ────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(GLEW   REQUIRED)
find_package(OpenSSL REQUIRED)    # Required by httplib for HTTPS

# ── Third-party via FetchContent ────────────────────────────────────────────
include(FetchContent)

# nlohmann/json (header-only)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3)
FetchContent_MakeAvailable(json)

# httplib (header-only)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3)
FetchContent_MakeAvailable(httplib)

# ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.5)
FetchContent_MakeAvailable(imgui)

# ImPlot (charts)
FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        v0.17)
FetchContent_MakeAvailable(implot)

# ── ImGui + ImPlot static library ───────────────────────────────────────────
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL GLEW::GLEW)

# ── Main executable ──────────────────────────────────────────────────────────
add_executable(CryptoTracker
    main.cpp
    AppManager.cpp
    DataManager.cpp
    FavoritesManager.cpp
)

target_include_directories(CryptoTracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
    ${httplib_SOURCE_DIR}
)

target_compile_definitions(CryptoTracker PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(CryptoTracker PRIVATE
    imgui_lib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    glfw
    OpenGL::GL
    GLEW::GLEW
)

# Enable warnings
target_compile_options(CryptoTracker PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -Wpedantic>
)
