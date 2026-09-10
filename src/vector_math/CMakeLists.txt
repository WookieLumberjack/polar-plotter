add_library(vector_math STATIC
    src/vec2.cpp)

target_include_directories(vector_math PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(vector_math PUBLIC cxx_std_23)

target_link_libraries(vector_math PRIVATE polar_plotter::warnings)

add_library(polar_plotter::vector_math ALIAS vector_math)
