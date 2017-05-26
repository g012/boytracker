set(CMAKE_LEGACY_CYGWIN_WIN32 OFF)
cmake_minimum_required(VERSION 3.1)

project(BoyTracker)

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
        ${BTK_SOURCE_DIR}/tinyfiledialogs.c
    )
set(BTK_HEADERS
        ${BTK_SOURCE_DIR}/Blip_Buffer.h
        ${BTK_SOURCE_DIR}/dr_path.h
        ${BTK_SOURCE_DIR}/imconfig.h
        ${BTK_SOURCE_DIR}/imgui.h
        ${BTK_SOURCE_DIR}/imgui_impl_glfw.h
        ${BTK_SOURCE_DIR}/imgui_internal.h
        ${BTK_SOURCE_DIR}/imgui_user.h
        ${BTK_SOURCE_DIR}/imgui_user.inl
        ${BTK_SOURCE_DIR}/ofl_gbapu.h
        ${BTK_SOURCE_DIR}/lua.h
        ${BTK_SOURCE_DIR}/parg.h
        ${BTK_SOURCE_DIR}/stb_rect_pack.h
        ${BTK_SOURCE_DIR}/stb_textedit.h
        ${BTK_SOURCE_DIR}/stb_truetype.h
        ${BTK_SOURCE_DIR}/tinyfiledialogs.h
    )
if (MSVC)
    set(BTK_RESOURCES res/win32/project.rc)
endif()
add_executable(${PROJECT_NAME} ${BTK_SOURCES} ${BTK_HEADERS} ${BTK_RESOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_include_directories(${PROJECT_NAME} PRIVATE "${BTK_SOURCE_DIR}" "${BTK_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} glfw)

if (MSVC)

    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /MANIFEST:NO")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    target_link_libraries(${PROJECT_NAME} opengl32)

elseif (APPLE)

    set(MACOSX_BUNDLE_BUNDLE_VERSION "${BTK_VERSION}")
    set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT_NAME})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.cuborph.${PROJECT_NAME}")
    set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "Application")
    set(MACOSX_BUNDLE_ICON_FILE "carton")
    set(MACOSX_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${BTK_VERSION}")

elseif (UNIX)

    target_link_libraries(${PROJECT_NAME} m X11 Xrandr rt GL GLU pthread dl Xinerama Xcursor)

endif()


install(TARGETS ${PROJECT_NAME} DESTINATION bin)

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${BTK_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${BTK_VERSION_MINOR}")
include(CPack)
