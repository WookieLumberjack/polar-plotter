add_library(ui STATIC
    src/angle_convention.cpp
    src/app.cpp
    src/config.cpp
    src/plot_plan.cpp
    src/zero_direction.cpp)

target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(ui PUBLIC cxx_std_23)

target_link_libraries(ui
    PUBLIC
        polar_plotter::vector_math
        polar_plotter::polar_plotting
        imgui::imgui
    PRIVATE
        polar_plotter::warnings)

add_library(polar_plotter::ui ALIAS ui)
