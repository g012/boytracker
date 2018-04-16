cmake_minimum_required(VERSION 3.1)

project(BoyTracker)

option(AUDIO_WINDOWS_DS "Build Windows DirectSound API" OFF)
option(AUDIO_WINDOWS_ASIO "Build Windows ASIO API" OFF)
option(AUDIO_WINDOWS_WASAPI "Build Windows WASAPI API" OFF)
option(AUDIO_LINUX_OSS "Build Linux OSS API" OFF)
option(AUDIO_LINUX_ALSA "Build Linux ALSA API" OFF)
option(AUDIO_LINUX_PULSE "Build Linux PulseAudio API" OFF)
option(AUDIO_UNIX_JACK "Build Unix JACK audio server API" OFF)
option(AUDIO_OSX_CORE "Build Mac OSX CoreAudio API" OFF)

include(CheckFunctionExists)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
add_subdirectory(src/glfw)

set(BTK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/btk)
set(BTK_BINARY_DIR ${PROJECT_BINARY_DIR}/src/btk)

set(BTK_VERSION_MAJOR "0")
set(BTK_VERSION_MINOR "1")
set(BTK_VERSION "${BTK_VERSION_MAJOR}.${BTK_VERSION_MINOR}")
configure_file("${BTK_SOURCE_DIR}/buildcfg.h.in" "${BTK_BINARY_DIR}/buildcfg.h")

add_definitions(-DGLEW_STATIC)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D__RTAUDIO_DEBUG__)
endif ()

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
if (HAVE_GETTIMEOFDAY)
    add_definitions(-DHAVE_GETTIMEOFDAY)
endif ()

if (MSVC)

    add_definitions(-D_HAS_EXCEPTIONS=0 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
    foreach (flag CMAKE_C_FLAGS
                  CMAKE_C_FLAGS_DEBUG
                  CMAKE_C_FLAGS_RELEASE
                  CMAKE_C_FLAGS_MINSIZEREL
                  CMAKE_C_FLAGS_RELWITHDEBINFO
                  CMAKE_CXX_FLAGS
                  CMAKE_CXX_FLAGS_DEBUG
                  CMAKE_CXX_FLAGS_RELEASE
                  CMAKE_CXX_FLAGS_MINSIZEREL
                  CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if (${flag} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
        endif()
        if (${flag} MATCHES "/MDd")
            string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
        endif()
        if(${flag} MATCHES "/EHsc")
            string(REGEX REPLACE "/EHsc" "/EHs-c-" ${flag} "${${flag}}")
        endif()
        if(${flag} MATCHES "/GR")
            string(REGEX REPLACE "/GR" "/GR-" ${flag} "${${flag}}")
        endif()
        if(${flag} MATCHES "/Gm")
            string(REGEX REPLACE "/Gm" "/Gm-" ${flag} "${${flag}}")
        endif()
    endforeach()

    add_compile_options(/WX /MP /fp:fast /volatile:iso)
    add_compile_options(/wd4103 /wd4244 /wd4456 /wd4554 /wd4557 /wd4702 /wd6255 /wd6262 /wd28278)

else ()

	add_definitions(-D_FILE_OFFSET_BITS=64)

    add_compile_options(-fno-strict-aliasing -fomit-frame-pointer -ffast-math -fvisibility=hidden -fno-threadsafe-statics -fvisibility-inlines-hidden)
    add_compile_options(-fno-rtti -fno-exceptions)
    add_compile_options(-Werror -Wsign-compare -Wno-dangling-else -Wno-missing-braces -Wno-unused-function -Wno-shift-op-parentheses -Wno-unused-variable -Wno-switch -Wno-parentheses)

endif()

set(BTK_SOURCES 
        ${BTK_SOURCE_DIR}/Blip_Buffer.cpp
        ${BTK_SOURCE_DIR}/csrc.c
        ${BTK_SOURCE_DIR}/editor.cpp
        ${BTK_SOURCE_DIR}/gl3w.c
        ${BTK_SOURCE_DIR}/glew.c
        ${BTK_SOURCE_DIR}/imgui.cpp
        ${BTK_SOURCE_DIR}/imgui_demo.cpp
        ${BTK_SOURCE_DIR}/imgui_draw.cpp
        ${BTK_SOURCE_DIR}/imgui_impl_glfw.cpp
        ${BTK_SOURCE_DIR}/lua.c
        ${BTK_SOURCE_DIR}/main.cpp
        ${BTK_SOURCE_DIR}/parg.c
        ${BTK_SOURCE_DIR}/player.cpp
        ${BTK_SOURCE_DIR}/RtAudio.cpp
        ${BTK_SOURCE_DIR}/RtMidi.cpp
        ${BTK_SOURCE_DIR}/tinyfiledialogs.c
    )
set(BTK_HEADERS
        ${BTK_SOURCE_DIR}/Blip_Buffer.h
        ${BTK_SOURCE_DIR}/dr_path.h
		${BTK_BINARY_DIR}/buildcfg.h
        ${BTK_SOURCE_DIR}/imconfig.h
        ${BTK_SOURCE_DIR}/imgui.h
        ${BTK_SOURCE_DIR}/imgui_impl_glfw.h
        ${BTK_SOURCE_DIR}/imgui_internal.h
        ${BTK_SOURCE_DIR}/imgui_user.h
        ${BTK_SOURCE_DIR}/imgui_user.inl
        ${BTK_SOURCE_DIR}/ofl_gbapu.h
        ${BTK_SOURCE_DIR}/lua.h
        ${BTK_SOURCE_DIR}/parg.h
        ${BTK_SOURCE_DIR}/player.h
        ${BTK_SOURCE_DIR}/stb_rect_pack.h
        ${BTK_SOURCE_DIR}/stb_textedit.h
        ${BTK_SOURCE_DIR}/stb_truetype.h
        ${BTK_SOURCE_DIR}/tinyfiledialogs.h
    )
if (MSVC)
    set(BTK_RESOURCES res/win32/project.rc)
endif()

set(LINKLIBS)
if (CMAKE_SYSTEM_NAME MATCHES "BSD")
    message(STATUS "BSD detected, using OSS")    
    find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
    list(APPEND LINKLIBS ossaudio ${CMAKE_THREAD_LIBS_INIT})
    set(AUDIO_LINUX_OSS ON)
elseif (UNIX AND NOT APPLE)
    if (NOT AUDIO_LINUX_PULSE AND NOT AUDIO_LINUX_ALSA AND NOT AUDIO_LINUX_OSS AND NOT AUDIO_UNIX_JACK)
        set(AUDIO_LINUX_ALSA ON)
    endif()
    if (AUDIO_LINUX_PULSE)
        find_library(PULSE_LIB pulse)
        find_library(PULSESIMPLE_LIB pulse-simple)
        find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
        list(APPEND LINKLIBS ${PULSE_LIB} ${PULSESIMPLE_LIB} ${CMAKE_THREAD_LIBS_INIT})
        add_definitions(-D__LINUX_PULSE__)
        message(STATUS "Using Linux PulseAudio")
    endif (AUDIO_LINUX_PULSE)
    if (AUDIO_LINUX_ALSA)
        find_package(ALSA)
        find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
        if (NOT ALSA_FOUND)
            message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
        endif()
        include_directories(${ALSA_INCLUDE_DIR})
        list(APPEND LINKLIBS ${ALSA_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
        add_definitions(-D__LINUX_ALSA__)
        message(STATUS "Using Linux ALSA")
    endif (AUDIO_LINUX_ALSA)
endif ()
if (APPLE)
    if (NOT AUDIO_OSX_CORE AND NOT AUDIO_UNIX_JACK)
        set(AUDIO_OSX_CORE ON)
    endif()

    if (AUDIO_OSX_CORE)
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREMIDI_LIB CoreMIDI)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        find_library(AUDIOTOOLBOX_LIB AudioToolbox)
        find_library(AUDIOUNIT_LIB AudioUnit)
        list(APPEND LINKLIBS ${COREAUDIO_LIB} ${COREMIDI_LIB} ${COREFOUNDATION_LIB} ${AUDIOTOOLBOX_LIB} ${AUDIOUNIT_LIB})
        add_definitions(-D__MACOSX_CORE__ -D__MACOSX_AU__)
        message(STATUS "Using OSX CoreAudio")
    endif (AUDIO_OSX_CORE)
endif ()

# JACK supported on many Unices
if (UNIX)
    if (AUDIO_UNIX_JACK)
        find_library(JACK_LIB jack)
        list(APPEND LINKLIBS ${JACK_LIB})
        add_definitions(-D__UNIX_JACK__)
        message(STATUS "Using JACK")
    endif (AUDIO_UNIX_JACK)
endif (UNIX)

if (WIN32)
    if (NOT AUDIO_WINDOWS_DS AND NOT AUDIO_WINDOWS_ASIO AND NOT AUDIO_WINDOWS_WASAPI)
        set(AUDIO_WINDOWS_WASAPI ON)
    endif()

    list(APPEND LINKLIBS winmm ole32)
    add_definitions(-D__WINDOWS_MM__)

    if (AUDIO_WINDOWS_DS)
        add_definitions(-D__WINDOWS_DS__)
        message(STATUS "Using Windows DirectSound")
        list(APPEND LINKLIBS dsound)
    endif (AUDIO_WINDOWS_DS)
    if (AUDIO_WINDOWS_WASAPI)
        add_definitions(-D__WINDOWS_WASAPI__)
        message(STATUS "Using Windows WASAPI")
        list(APPEND LINKLIBS uuid ksuser)
    endif (AUDIO_WINDOWS_WASAPI)
    if (AUDIO_WINDOWS_ASIO)
        list(APPEND BTK_SOURCES
            ${BTK_SOURCE_DIR}/asio.cpp
            ${BTK_SOURCE_DIR}/asiodrivers.cpp
            ${BTK_SOURCE_DIR}/asiolist.cpp
            ${BTK_SOURCE_DIR}/iasiothiscallresolver.cpp)
        add_definitions(-D__WINDOWS_ASIO__)
        message(STATUS "Using Windows ASIO")
    endif (AUDIO_WINDOWS_ASIO)
endif (WIN32)

add_executable(${PROJECT_NAME} ${BTK_SOURCES} ${BTK_HEADERS} ${BTK_RESOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_include_directories(${PROJECT_NAME} PRIVATE "${BTK_SOURCE_DIR}" "${BTK_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} glfw)

if (MSVC)

    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /MANIFEST:NO /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    list(APPEND LINKLIBS opengl32)

elseif (APPLE)

    set(MACOSX_BUNDLE_BUNDLE_VERSION "${BTK_VERSION}")
    set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT_NAME})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.cuborph.${PROJECT_NAME}")
    set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "Application")
    set(MACOSX_BUNDLE_ICON_FILE "carton")
    set(MACOSX_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${BTK_VERSION}")

elseif (UNIX)

    list(APPEND LINKLIBS m X11 Xrandr rt GL GLU dl Xinerama Xcursor)

endif()

target_link_libraries(${PROJECT_NAME} ${LINKLIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${BTK_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${BTK_VERSION_MINOR}")
include(CPack)
