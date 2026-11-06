# raw_opengl_demo(<target> <sources...>): a demo in raw OpenGL 3.3, native and web.
#
#   Native: OpenGL 3.3 core, GLFW (installed: brew install glfw, apt install libglfw3-dev), glad.
#   Web:    emcmake cmake -S . -B build_web && cmake --build build_web
#           gives project.js and project.wasm, and index.html (bridge/slide-frame.html) next to them,
#           then copies these three files into web/ (the folder the slides load) and, if it exists, into
#           lhtml-site/src/_demo/: after a build, the slides show the new version of the program.
#           To keep web/ as it is: emcmake cmake -S . -B build_web -DSLIDE_DEMO_PUBLISH=OFF

set(INTERACTIVE_3D_SLIDES_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(SLIDE_BRIDGE_DIR "${INTERACTIVE_3D_SLIDES_ROOT}/bridge")
set(SLIDE_GLAD_DIR "${INTERACTIVE_3D_SLIDES_ROOT}/examples/third_party/glad")
option(SLIDE_DEMO_PUBLISH "Copy the web build into web/ and lhtml-site/src/_demo/" ON)

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
        if(SLIDE_DEMO_PUBLISH)
            set(built "$<TARGET_FILE_DIR:${target}>/index.html" "$<TARGET_FILE_DIR:${target}>/project.js"
                      "$<TARGET_FILE_DIR:${target}>/project.wasm")
            set(web "${CMAKE_CURRENT_SOURCE_DIR}/web")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${web}"
                COMMAND ${CMAKE_COMMAND} -E copy ${built} "${web}/"
                COMMENT "web build copied into web/")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lhtml-site")
                # the lhtml-site slide also needs the poster (its image in a PDF) and slide-controls.js
                set(demo "${CMAKE_CURRENT_SOURCE_DIR}/lhtml-site/src/_demo")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${demo}"
                    COMMAND ${CMAKE_COMMAND} -E copy ${built} "${SLIDE_BRIDGE_DIR}/slide-controls.js"
                            "${CMAKE_CURRENT_SOURCE_DIR}/poster.png" "${demo}/"
                    COMMENT "web build copied into lhtml-site/src/_demo/")
            endif()
        endif()
    else()
        find_package(glfw3 3.3 REQUIRED)
        find_package(OpenGL REQUIRED)
        target_sources(${target} PRIVATE "${SLIDE_GLAD_DIR}/glad.cpp")
        target_include_directories(${target} PRIVATE "${SLIDE_GLAD_DIR}")
        target_link_libraries(${target} PRIVATE glfw OpenGL::GL ${CMAKE_DL_LIBS})
    endif()
endfunction()
