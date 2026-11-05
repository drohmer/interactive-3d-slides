# raw_opengl_demo(<target> <sources...>): a demo in raw OpenGL 3.3, native and web.
#
#   Native: OpenGL 3.3 core, GLFW (installed: brew install glfw, apt install libglfw3-dev), glad.
#   Web:    emcmake cmake -S . -B build_web && cmake --build build_web
#           gives project.js and project.wasm, and index.html (bridge/slide-frame.html) next to them.

set(SLIDE_DEMOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(SLIDE_BRIDGE_DIR "${SLIDE_DEMOS_ROOT}/bridge")
set(SLIDE_GLAD_DIR "${SLIDE_DEMOS_ROOT}/examples/third_party/glad")

function(raw_opengl_demo target)
    add_executable(${target} ${ARGN} "${SLIDE_BRIDGE_DIR}/slide_bridge.cpp")
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} PRIVATE "${SLIDE_BRIDGE_DIR}")

    if(EMSCRIPTEN)
        target_link_options(${target} PRIVATE
            -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
            -sEXPORTED_FUNCTIONS=_main,_slide_set,_slide_describe
            -sEXPORTED_RUNTIME_METHODS=ccall)
        # the page of the demo, for a slide
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${SLIDE_BRIDGE_DIR}/slide-frame.html" "$<TARGET_FILE_DIR:${target}>/index.html")
    else()
        find_package(glfw3 3.3 REQUIRED)
        find_package(OpenGL REQUIRED)
        target_sources(${target} PRIVATE "${SLIDE_GLAD_DIR}/glad.cpp")
        target_include_directories(${target} PRIVATE "${SLIDE_GLAD_DIR}")
        target_link_libraries(${target} PRIVATE glfw OpenGL::GL ${CMAKE_DL_LIBS})
    endif()
endfunction()
