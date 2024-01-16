if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    set(BSD true)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig)

include_directories("${PROJECT_SOURCE_DIR}/")

file (GLOB SRC
    "${PROJECT_SOURCE_DIR}/main_src/main.c"
    "${PROJECT_SOURCE_DIR}/common/game.c"
)

# frontends

# compile vulkan shaders
add_custom_command(
  OUTPUT "${PROJECT_SOURCE_DIR}/assets/msboard4vs.spv"
  COMMAND "glslc"
  ARGS "${PROJECT_SOURCE_DIR}/assets/msboard4.vert" "-o" "${PROJECT_SOURCE_DIR}/assets/msboard4vs.spv"
  DEPENDS "${PROJECT_SOURCE_DIR}/assets/msboard4.vert"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(
  OUTPUT "${PROJECT_SOURCE_DIR}/assets/msboard4fs.spv"
  COMMAND "glslc"
  ARGS "${PROJECT_SOURCE_DIR}/assets/msboard4.frag" "-o" "${PROJECT_SOURCE_DIR}/assets/msboard4fs.spv"
  DEPENDS "${PROJECT_SOURCE_DIR}/assets/msboard4.frag"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/console.c")
add_compile_definitions(FRONTEND_CONSOLE)
message(STATUS "Building with console")

if (UNIX)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/httpd.c")
    add_compile_definitions(FRONTEND_HTTPD)
    message(STATUS "Building with httpd")
else()
    message(STATUS "Not building with console")
endif (UNIX)

if (WIN32)
    message(STATUS "No fbdev support in Win32")
elseif (BSD)
    message(STATUS "No fbdev support in BSD")
else()
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/fbdev.c")
    add_compile_definitions(FRONTEND_FBDEV)
    message(STATUS "Building with fbdev")
endif()


find_package(X11)
if (X11_FOUND)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/xlib.c")
    add_compile_definitions(FRONTEND_XLIB)
    message(STATUS "Building with X11 Xlib")

    if (X11_xcb_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/xcb.c")
        add_compile_definitions(FRONTEND_XCB)
        message(STATUS "Building with xcb")
    else()
        message(STATUS "Not building with xcb")
    endif()

    find_package(Motif)
    if (MOTIF_FOUND)
        file(GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/motif.c")
        add_compile_definitions(FRONTEND_MOTIF)
        message(STATUS "Building with motif")
    else()
        message(STATUS "Not building with motif")
    endif()

    if (EXISTS "/usr/include/forms.h")
        set(XFORMS_FOUND true)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/xforms.c")
        add_compile_definitions(FRONTEND_XFORMS)
        message(STATUS "Building with XForms")
    else()
        message(STATUS "Not Building with XForms")
    endif()

else()
    message(STATUS "Not building with X11 Xlib")
endif()

#find_package(Wayland)
pkg_check_modules(WAYLAND wayland-client) 
if (WAYLAND_FOUND)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/wayland.c")
    add_compile_definitions(FRONTEND_WAYLAND)
    message(STATUS "Building with Wayland")
else()
    message(STATUS "Not building with Wayland")
endif()

pkg_check_modules(GTK3 gtk+-3.0) 
if (GTK3_FOUND)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/gtk3.c")
    add_compile_definitions(FRONTEND_GTK3)
    message(STATUS "Building with gtk3")
else()
    message(STATUS "Not building with gtk3")
endif()

find_package(Qt5Widgets)
if (Qt5Widgets_FOUND)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/qt5.cpp" "${PROJECT_SOURCE_DIR}/main_src/frontends/qt5.hpp")
    include_directories(${Qt5Widgets_INCLUDES})
    add_definitions(${Qt5Widgets_DEFINITIONS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    add_compile_definitions(FRONTEND_QT5)
    message(STATUS "Building with Qt5")
else()
    message(STATUS "Not building with Qt5")
endif()

find_package(SDL2)
pkg_check_modules(SDL2_ttf SDL2_ttf)
pkg_check_modules(SDL2_image SDL2_image)
if (SDL2_FOUND)
    if (SDL2_ttf_FOUND AND SDL2_image_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/sdl2.c")
        add_compile_definitions(FRONTEND_SDL2)
        message(STATUS "Building with SDL2")
    else()
        message(WARNING "SDL2_ttf and SDL2_image required for building SDL2 frontend")
        message(STATUS "Not building with SDL2")
    endif()
else()
    message(STATUS "Not building with SDL2")
endif()

find_package(SDL)
pkg_check_modules(SDL_ttf SDL_ttf)
pkg_check_modules(SDL_image SDL_image)
if (SDL_FOUND)
    if (SDL_ttf_FOUND AND SDL_image_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/sdl1.c")
        add_compile_definitions(FRONTEND_SDL1)
        message(STATUS "Building with SDL 1.2")
    else()
        message(WARNING "SDL_ttf and SDL_image required for building SDL1.2 frontend")
        message(STATUS "Not building with SDL 1.2")
    endif()
else()
    message(STATUS "Not building with SDL 1.2")
endif()

cmake_policy(SET CMP0072 OLD)
find_package(OpenGL)
find_package(glfw3 3.3)
if (OpenGL_OpenGL_FOUND)
    find_package(GLUT)
    if (GLUT_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/gl11.c")
        add_compile_definitions(FRONTEND_GL11)
        message(STATUS "Building with OpenGL 2.x FFP/FreeGLUT")
    else()
        message(WARNING "FreeGLUT required for building OpenGL 1.1 frontend")
        message(STATUS "Not building with OpenGL 2.x FFP/FreeGLUT")
    endif()

    if (glfw3_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/gl33.c" "${PROJECT_SOURCE_DIR}/main_src/frontends/glad/glad.c" "${PROJECT_SOURCE_DIR}/main_src/frontends/glutil.c")
        include_directories("${PROJECT_SOURCE_DIR}/main_src/frontends/glad/include/")
        add_compile_definitions(FRONTEND_GL33)
        message(STATUS "Building with OpenGL 3.3/GLFW/GLAD")
    else()
        message(WARNING "GLFW required for building OpenGL 3.3 frontend")
        message(STATUS "Not building with OpenGL 3.3/GLFW/GLAD")
    endif()
else()
    message(STATUS "Not building with OpenGL")
endif()

find_package(Vulkan)
if (Vulkan_FOUND)
    if (glfw3_FOUND)
        file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/vk.c")
        include_directories("${PROJECT_SOURCE_DIR}/main_src/frontends/glad/include/")
        add_compile_definitions(FRONTEND_VK)
        add_custom_target(vulkan_shaders ALL
            DEPENDS "${PROJECT_SOURCE_DIR}/assets/msboard4vs.spv" "${PROJECT_SOURCE_DIR}/assets/msboard4fs.spv"
        )
        message(STATUS "Building with Vulkan/GLFW")
    else()
        message(WARNING "GLFW required for building Vulkan frontend")
        message(STATUS "Not building with Vulkan/GLFW")
    endif()
else()
    message(STATUS "Not building with Vulkan/GLFW")
endif()

if (WIN32)
    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/win32.c")
    add_compile_definitions(FRONTEND_WIN32)
    message(STATUS "Building with Win32 ComCtl")

    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/gdi.c")
    add_compile_definitions(FRONTEND_GDI)
    message(STATUS "Building with Win32 GDI")

    file (GLOB SRC ${SRC} "${PROJECT_SOURCE_DIR}/main_src/frontends/d2d.c")
    add_compile_definitions(FRONTEND_D2D)
    message(STATUS "Building with Win32 Direct2D")
else()
    message(STATUS "Not building with Win32")
endif()

# executable
add_executable(arfminesweeper ${SRC})


# link libraries

target_link_libraries(arfminesweeper Threads::Threads m)

if (WIN32)
    target_link_libraries(arfminesweeper ws2_32 msimg32 D2d1)
endif()

if (X11_FOUND)
    target_link_libraries(arfminesweeper ${X11_LIBRARIES})

    if (X11_xcb_FOUND)
        target_link_libraries(arfminesweeper X11::xcb)
    endif()

    if (MOTIF_FOUND)
        target_link_libraries(arfminesweeper ${MOTIF_LIBRARIES} Xt)
    endif()

    if (XFORMS_FOUND)
        target_link_libraries(arfminesweeper forms)
    endif()
endif()

if (GTK3_FOUND)
    target_include_directories(arfminesweeper PUBLIC ${GTK3_INCLUDE_DIRS})
    target_link_libraries(arfminesweeper ${GTK3_LIBRARIES})
endif()

if (Qt5Widgets_FOUND)
    target_link_libraries(arfminesweeper Qt5::Widgets)
endif()

if (SDL2_FOUND AND SDL2_ttf_FOUND AND SDL2_image_FOUND)
    target_link_libraries(arfminesweeper ${SDL2_LIBRARIES} ${SDL2_ttf_LIBRARIES} ${SDL2_image_LIBRARIES})
endif()

if (SDL_FOUND AND SDL_ttf_FOUND AND SDL_image_FOUND)
    target_link_libraries(arfminesweeper ${SDL_LIBRARIES} ${SDL_ttf_LIBRARIES} ${SDL_image_LIBRARIES})
endif()

if (OpenGL_OpenGL_FOUND)
    target_link_libraries(arfminesweeper OpenGL::GL)

    if (GLUT_FOUND)
        target_link_libraries(arfminesweeper ${GLUT_LIBRARIES})
    endif()

    if (glfw3_FOUND)
        target_link_libraries(arfminesweeper glfw dl)
    endif()
endif()

if (Vulkan_FOUND AND glfw3_FOUND)
    target_link_libraries(arfminesweeper Vulkan::Vulkan)
endif()

if (WAYLAND_FOUND)
    target_link_libraries(arfminesweeper wayland-client)
endif()

# install
install(TARGETS arfminesweeper RUNTIME DESTINATION bin)

# package
include(CPack)
