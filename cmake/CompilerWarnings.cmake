# An INTERFACE target carrying the project's warning policy.
# Link it PRIVATE-ly into first-party targets only; never into third-party code.

add_library(project_warnings INTERFACE)
add_library(polar_plotter::warnings ALIAS project_warnings)

if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # clang-cl / MSVC front end
    target_compile_options(project_warnings INTERFACE
        /W4 /permissive- /WX)
else()
    target_compile_options(project_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wdouble-promotion
        -Wformat=2)
endif()

option(POLAR_PLOTTER_WERROR "Treat warnings as errors in first-party code" ON)
if(NOT POLAR_PLOTTER_WERROR)
    if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(project_warnings INTERFACE /WX-)
    else()
        target_compile_options(project_warnings INTERFACE -Wno-error)
    endif()
endif()
