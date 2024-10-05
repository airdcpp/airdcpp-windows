cmake_minimum_required(VERSION 3.22)

project(
    "airdcpp-windows"
    DESCRIPTION "Communal peer-to-peer file sharing application"
)


set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set (PROJECT_NAME "AirDC++")
set (TAG_APPLICATION "AirDC++")
set (APPLICATION_ID "airdcpp-windows")
set (VERSION "3.22b")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# global MSVC runtime linking for all project targets
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# global debug postfix for libraries (executables still need to set it)
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Filename postfix for libraries under DEBUG configuration")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")



add_compile_options("/permissive-")

# The unofficial natpmp port has headers incorrectly inside a subdirectory
include_directories (
	compiled/x64-release/vcpkg_installed/x64-windows-static/include/natpmp
	compiled/x64-debug/vcpkg_installed/x64-windows-static/include/natpmp
	compiled/x86-release/vcpkg_installed/x86-windows-static/include/natpmp
	compiled/x86-debug/vcpkg_installed/x86-windows-static/include/natpmp
)

add_subdirectory(airdcpp)
add_subdirectory(airdcpp-webapi)
add_subdirectory(windows)
