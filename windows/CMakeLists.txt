project (airdcpp-wingui)
cmake_minimum_required (VERSION 3.16)

find_path(WTL_INCLUDE_DIRS "atlapp.h")

file (GLOB_RECURSE wingui_hdrs ${PROJECT_SOURCE_DIR}/*.h)
file (GLOB_RECURSE wingui_srcs ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.c)

unset(CMAKE_INSTALL_BINDIR)
add_executable(${PROJECT_NAME} WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "AirDC")

set(resources
    AirDC.rc
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${wingui_srcs}
        ${resources}
)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)



# VC++ Directories > Include Directories
set(CMAKE_VS_SDK_INCLUDE_DIRECTORIES "$(VC_IncludePath);$(WindowsSDK_IncludePath)")

target_precompile_headers(${PROJECT_NAME} PUBLIC
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/windows/stdafx.h>"
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        LINK_FLAGS
            "/NODEFAULTLIB:libcmt.lib;kernel32.lib;advapi32.lib;user32.lib;gdi32.lib;shell32.lib;comdlg32.lib;version.lib;mpr.lib;rasapi32.lib;winmm.lib;winspool.lib;vfw32.lib;secur32.lib;oleacc.lib;oledlg.lib;sensapi.lib \
            /MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\""
)


target_compile_definitions(${PROJECT_NAME}
    PRIVATE
		"_WTL_NEW_PAGE_NOTIFY_HANDLERS"
        "HAVE_NATUPNP_H"
        "_UNICODE"
        "UNICODE"
)



# LINKING
target_link_libraries (
	${PROJECT_NAME}
	
	airdcpp
	airdcpp-webapi
	
	WinMM # PlaySound
	powrprof # SetSuspendState
)

install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
install(TARGETS ${PROJECT_NAME})

