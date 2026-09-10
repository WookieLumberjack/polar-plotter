add_library(polar_plotting STATIC
    src/polar_plot.cpp)

target_include_directories(polar_plotting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(polar_plotting PUBLIC cxx_std_23)

# Reusable outside this project: depends only on Dear ImGui + ImPlot.
target_link_libraries(polar_plotting
    PUBLIC implot::implot
    PRIVATE polar_plotter::warnings)

add_library(polar_plotter::polar_plotting ALIAS polar_plotting)
