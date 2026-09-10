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
# future-standard extension on newer Clang (e.g. MSYS2 clang64). Suppress it
# only where the compiler actually knows the flag -- older Clang errors on an
# unknown -Wno-* under -Werror. The project warning policy still applies to
# everything else in the test TU.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wc2y-extensions POLAR_PLOTTER_HAS_WC2Y)
if(POLAR_PLOTTER_HAS_WC2Y)
    target_compile_options(vector_math_tests PRIVATE -Wno-c2y-extensions)
endif()

catch_discover_tests(vector_math_tests)
