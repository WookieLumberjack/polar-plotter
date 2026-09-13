include(Catch)

add_executable(vector_math_tests
    vec2_tests.cpp)

target_link_libraries(vector_math_tests
    PRIVATE
        polar_plotter::vector_math
        polar_plotter::warnings
        Catch2::Catch2WithMain)

target_compile_features(vector_math_tests PRIVATE cxx_std_23)

# Catch2's TEST_CASE macro expands __COUNTER__, which -Wpedantic flags as a
# future-standard extension on newer Clang (e.g. MSYS2 clang64). The suppression
# has to sit *after* the interface -Wpedantic on the command line, so it goes on
# the source file (source options are emitted last) rather than the target.
# -Wno-unknown-warning-option (every Clang knows it) keeps older Clang, which
# lacks -Wc2y-extensions, from erroring on the unknown flag under -Werror.
if(NOT MSVC)
    set_source_files_properties(vec2_tests.cpp PROPERTIES
        COMPILE_OPTIONS "-Wno-unknown-warning-option;-Wno-c2y-extensions")
endif()

catch_discover_tests(vector_math_tests)

add_executable(polar_plotting_tests
    polar_plot_tests.cpp)

target_link_libraries(polar_plotting_tests
    PRIVATE
        polar_plotter::polar_plotting
        polar_plotter::warnings
        Catch2::Catch2WithMain)

target_compile_features(polar_plotting_tests PRIVATE cxx_std_23)

# See the comment above vec2_tests.cpp's identical suppression.
if(NOT MSVC)
    set_source_files_properties(polar_plot_tests.cpp PROPERTIES
        COMPILE_OPTIONS "-Wno-unknown-warning-option;-Wno-c2y-extensions")
endif()

catch_discover_tests(polar_plotting_tests)

add_executable(ui_tests
    ui_angle_convention_tests.cpp
    construction_tests.cpp
    config_tests.cpp
    derived_vectors_tests.cpp
    plot_plan_tests.cpp
    polar_display_tests.cpp
    theme_tests.cpp
    vector_palette_tests.cpp
    zero_direction_tests.cpp)

target_link_libraries(ui_tests
    PRIVATE
        polar_plotter::ui
        polar_plotter::polar_plotting
        polar_plotter::vector_math
        polar_plotter::warnings
        Catch2::Catch2WithMain)

target_compile_features(ui_tests PRIVATE cxx_std_23)

# See the comment above vec2_tests.cpp's identical suppression.
if(NOT MSVC)
    set_source_files_properties(ui_angle_convention_tests.cpp construction_tests.cpp config_tests.cpp derived_vectors_tests.cpp plot_plan_tests.cpp polar_display_tests.cpp theme_tests.cpp vector_palette_tests.cpp zero_direction_tests.cpp PROPERTIES
        COMPILE_OPTIONS "-Wno-unknown-warning-option;-Wno-c2y-extensions")
endif()

catch_discover_tests(ui_tests)
