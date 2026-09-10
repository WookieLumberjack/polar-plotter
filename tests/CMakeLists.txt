include(Catch)

add_executable(vector_math_tests
    vec2_tests.cpp)

target_link_libraries(vector_math_tests
    PRIVATE
        polar_plotter::vector_math
        polar_plotter::warnings
        Catch2::Catch2WithMain)

target_compile_features(vector_math_tests PRIVATE cxx_std_23)

catch_discover_tests(vector_math_tests)
