add_executable(polar_plotter_app
    main.cpp)

set_target_properties(polar_plotter_app PROPERTIES
    OUTPUT_NAME polar-plotter)

target_compile_features(polar_plotter_app PRIVATE cxx_std_23)

add_dependencies(polar_plotter_app generate_jetbrains_mono_header)
target_include_directories(polar_plotter_app PRIVATE ${CMAKE_BINARY_DIR}/generated)

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
    # Static-link the MSYS2 clang64 runtime (libc++/libunwind/libwinpthread)
    # so the released .exe has no MSYS2 DLL dependencies -- see the release
    # workflow, which ships this binary standalone rather than alongside its
    # runtime DLLs.
    target_link_options(polar_plotter_app PRIVATE -static)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(polar_plotter_app PRIVATE OpenGL::GL)
endif()
