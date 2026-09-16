add_executable(polar_plotter_app
    main.cpp
    # Compiled on every platform (see cmake/Dependencies.cmake's
    # Vulkan-Headers/volk section for why); only ever invoked on Windows.
    vulkan_backend.cpp)

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
        volk
        Vulkan::Headers
        polar_plotter::warnings)

if(APPLE)
    target_link_libraries(polar_plotter_app PRIVATE "-framework OpenGL")
    # macOS deprecated the OpenGL API in 10.14; we knowingly use the compat GL 1.1
    # entry points that Dear ImGui's backend also relies on.
    target_compile_definitions(polar_plotter_app PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
    # No opengl32 link: Windows renders via the Vulkan path in
    # vulkan_backend.cpp (see docs/adr/0004-windows-vulkan-hard-cutover.md),
    # not app/main.cpp's OpenGL branch. imgui::imgui still links opengl32
    # transitively for its imgui_impl_opengl3.cpp translation unit, which
    # stays compiled (unused on Windows) for the other two platforms' sake.
    #
    # Static-link the MSYS2 clang64 runtime (libc++/libunwind/libwinpthread)
    # so the released .exe has no MSYS2 DLL dependencies -- see the release
    # workflow, which ships this binary standalone rather than alongside its
    # runtime DLLs.
    target_link_options(polar_plotter_app PRIVATE -static)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(polar_plotter_app PRIVATE OpenGL::GL)
endif()
