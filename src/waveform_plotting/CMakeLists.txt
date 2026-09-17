add_library(waveform_plotting STATIC
    src/waveform_buffer.cpp
    src/draw_waveform.cpp)

target_include_directories(waveform_plotting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(waveform_plotting PUBLIC cxx_std_23)

# Reusable outside this project: depends only on Dear ImGui + ImPlot.
target_link_libraries(waveform_plotting
    PUBLIC implot::implot
    PRIVATE polar_plotter::warnings)

add_library(polar_plotter::waveform_plotting ALIAS waveform_plotting)
