cmake_minimum_required(VERSION 3.24)

project(polar_plotter
    VERSION 0.1.0
    DESCRIPTION "Interactive polar vector-math playground"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clang-tidy / editors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)  # defines BUILD_TESTING (default ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# Third-party dependencies (Dear ImGui, ImPlot, GLFW, Catch2) via FetchContent.
include(Dependencies)

# First-party modules. Keep the dependency direction one-way:
#   vector_math      -> (nothing)
#   polar_plotting   -> imgui, implot            (reusable, no vector_math)
#   waveform_plotting -> imgui, implot           (reusable, no vector_math)
#   ui               -> vector_math, polar_plotting, waveform_plotting, imgui, implot
add_subdirectory(src/vector_math)
add_subdirectory(src/polar_plotting)
add_subdirectory(src/waveform_plotting)
add_subdirectory(src/ui)

# The application executable.
add_subdirectory(app)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
