# polar-plotter

Interactive desktop tool for learning 2D vector math: enter vector quantities,
plot them on a polar canvas, and see vector operations (sum, difference, dot
product, angle between) update live. Built with Dear ImGui + ImPlot.

## Requirements

- Clang with C++23 support
- CMake >= 3.24
- Ninja
- OpenGL/X11 development libraries (Linux only — GLFW is built X11-only for now)

All other dependencies (GLFW 3.4, Dear ImGui v1.92.9b, ImPlot v1.0, Catch2
v3.7.1) are fetched and pinned automatically by `cmake/Dependencies.cmake` —
no need to install them separately.

Supported platforms: Linux, Windows (MSYS2 `clang64`), and macOS.

## Building

Configure and build with the provided CMake presets. There's a preset for
each build type — use whichever you need:

**Debug** (default for day-to-day development and testing):

```sh
cmake --preset debug
cmake --build --preset debug
```

**Release** (optimized, `RelWithDebInfo`):

```sh
cmake --preset release
cmake --build --preset release
```

Each preset configures its own build directory (`build/debug`,
`build/release`), so you can have both set up at once and rebuild either
independently.

If the unversioned `clang`/`clang++` isn't on your `PATH` (common on Linux),
point CMake at the versioned binaries instead, for either preset:

```sh
cmake --preset debug -D CMAKE_CXX_COMPILER=clang++-21 -D CMAKE_C_COMPILER=clang-21
cmake --preset release -D CMAKE_CXX_COMPILER=clang++-21 -D CMAKE_C_COMPILER=clang-21
```

There's also an `asan` preset (debug build instrumented with ASan + UBSan),
configured and built the same way:

```sh
cmake --preset asan
cmake --build --preset asan
```

## Running

After building, the executable is at `build/<preset>/app/polar-plotter`:

```sh
./build/debug/app/polar-plotter    # debug build
./build/release/app/polar-plotter  # release build
```

### Headless screenshot mode

For environments without a usable display, set `POLAR_PLOTTER_SCREENSHOT` to
render a few frames to a hidden window and dump the framebuffer as a PPM
image, then exit:

```sh
POLAR_PLOTTER_SCREENSHOT=out.ppm ./build/debug/app/polar-plotter
```

On a headless machine, wrap the command with `xvfb-run` and force the
software GL renderer:

```sh
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run POLAR_PLOTTER_SCREENSHOT=out.ppm ./build/debug/app/polar-plotter
```

Convert the PPM to a viewable format with ImageMagick:

```sh
magick out.ppm out.png
```

## Testing

Run the Catch2 test suite via CTest:

```sh
ctest --preset debug
```

Run a single test by name:

```sh
./build/debug/tests/vector_math_tests "<test name>"
```

or filter by regex through CTest:

```sh
ctest --preset debug -R <regex>
```
