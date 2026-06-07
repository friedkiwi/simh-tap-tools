cmake_minimum_required(VERSION 3.24)

project(simh_tap_tools
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as shared libraries" FORCE)
set(wxBUILD_MONOLITHIC OFF CACHE BOOL "Build wxWidgets as a single library" FORCE)
set(wxBUILD_SAMPLES OFF CACHE STRING "Build wxWidgets samples" FORCE)
set(wxBUILD_TESTS OFF CACHE STRING "Build wxWidgets tests" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "Build wxWidgets demos" FORCE)
set(wxUSE_AUI OFF CACHE BOOL "Use wxAUI" FORCE)
set(wxUSE_HELP OFF CACHE BOOL "Use wxWidgets help subsystem" FORCE)
set(wxUSE_HTML OFF CACHE BOOL "Use wxHTML" FORCE)
set(wxUSE_MEDIACTRL OFF CACHE BOOL "Use wxMediaCtrl" FORCE)
set(wxUSE_MS_HTML_HELP OFF CACHE BOOL "Use MS HTML Help" FORCE)
set(wxUSE_PROPGRID OFF CACHE BOOL "Use wxPropertyGrid" FORCE)
set(wxUSE_RIBBON OFF CACHE BOOL "Use wxRibbon" FORCE)
set(wxUSE_RICHTEXT OFF CACHE BOOL "Use wxRichTextCtrl" FORCE)
set(wxUSE_STC OFF CACHE BOOL "Use wxStyledTextCtrl" FORCE)
set(wxUSE_WEBVIEW OFF CACHE BOOL "Use wxWebView" FORCE)
set(wxUSE_WXHTML_HELP OFF CACHE BOOL "Use wxHTML-based help" FORCE)
set(wxUSE_XRC OFF CACHE BOOL "Use XRC resources" FORCE)

add_library(tap STATIC
    src/tap/Error.h
    src/tap/Reader.cpp
    src/tap/Reader.h
    src/tap/Result.h
    src/tap/Scanner.cpp
    src/tap/Scanner.h
    src/tap/SimhFormat.cpp
    src/tap/SimhFormat.h
    src/tap/TapeElement.cpp
    src/tap/TapeElement.h
    src/tap/TapeImage.cpp
    src/tap/TapeImage.h
    src/tap/Writer.cpp
    src/tap/Writer.h
)

target_include_directories(tap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(vendor/wxWidgets)

add_executable(simh-tap-tools WIN32
    src/App.cpp
    src/App.h
    src/MainFrame.cpp
    src/MainFrame.h
)

target_link_libraries(simh-tap-tools
    PRIVATE
        tap
        wx::base
        wx::core
)

add_executable(tap-tests
    tests/tap/TapTests.cpp
)

target_link_libraries(tap-tests
    PRIVATE
        tap
)

target_compile_definitions(tap-tests
    PRIVATE
        TAP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples"
)

add_test(NAME tap-tests COMMAND tap-tests)

if(MINGW)
    target_link_options(simh-tap-tools PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_options(tap-tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
