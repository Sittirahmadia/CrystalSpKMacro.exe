cmake_minimum_required(VERSION 3.20)
project(CrystalSpKMacroExe VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimize for speed
if(MSVC)
    add_compile_options(/O2 /Oi /GL /GS- /fp:fast /EHsc /W3)
    add_link_options(/LTCG /OPT:REF /OPT:ICF)
else()
    add_compile_options(-O3 -flto)
endif()

# Static runtime (single .exe, no VCRUNTIME DLL needed)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Common definitions
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp
    ${IMGUI_DIR}/imgui_impl_dx11.cpp
)

# Single GUI executable — ImGui + DX11 + all macros built-in
add_executable(CrystalSpKMacro WIN32
    src/main.cpp
    ${IMGUI_SOURCES}
)
target_include_directories(CrystalSpKMacro PRIVATE src src/imgui)
target_link_libraries(CrystalSpKMacro PRIVATE
    user32 gdi32 dwmapi kernel32 winmm
    d3d11 dxgi d3dcompiler
)
