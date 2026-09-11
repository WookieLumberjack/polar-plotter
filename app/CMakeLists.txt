add_executable(polar_plotter_app
    main.cpp)

set_target_properties(polar_plotter_app PROPERTIES
    OUTPUT_NAME polar-plotter)

target_compile_features(polar_plotter_app PRIVATE cxx_std_23)

target_link_libraries(polar_plotter_app
    PRIVATE
        polar_plotter::ui
        imgui::imgui
        implot::implot
        glfw
        polar_plotter::warnings)

if(APPLE)
    target_link_libraries(polar_plotter_app PRIVATE "-framework OpenGL")
    # macOS deprecated the OpenGL API in 10.14; we knowingly use the compat GL 1.1
    # entry points that Dear ImGui's backend also relies on.
    target_compile_definitions(polar_plotter_app PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
    target_link_libraries(polar_plotter_app PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(polar_plotter_app PRIVATE OpenGL::GL)
endif()
