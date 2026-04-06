cmake_minimum_required(VERSION 3.20)
project(CrystalSpKMacroExe VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimize for speed
if(MSVC)
    add_compile_options(/O2 /Oi /GL /GS- /fp:fast /EHsc /W3)
    add_link_options(/LTCG /OPT:REF /OPT:ICF)
else()
    add_compile_options(-O3 -march=native -flto)
endif()

# Static runtime (single .exe, no VCRUNTIME DLL needed)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Common definitions
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# ── GUI Application (CrystalSpKMacro.exe) ────────────────────────────────────
add_executable(CrystalSpKMacro WIN32 src/gui.cpp)
target_include_directories(CrystalSpKMacro PRIVATE src)
target_link_libraries(CrystalSpKMacro PRIVATE user32 gdi32 dwmapi kernel32 winmm)

# ── Individual macro executables ─────────────────────────────────────────────
set(MACROS sa da ap hc kp idh oht asb sr ls es pc ss bs ic xb dr lw la)

foreach(MACRO ${MACROS})
    add_executable(${MACRO} src/${MACRO}.cpp)
    target_include_directories(${MACRO} PRIVATE src)
    target_link_libraries(${MACRO} PRIVATE user32 kernel32 winmm)
endforeach()
