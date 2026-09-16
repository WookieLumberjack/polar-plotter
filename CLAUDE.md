# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Interactive desktop tool for learning 2D vector math: enter vector quantities,
plot them on a polar canvas, and see vector operations (sum, difference, dot
product, angle between) update live. Dear ImGui + ImPlot front end.

## Build & test

Requires Clang (C++23), CMake ≥ 3.24, Ninja. Dependencies are fetched and pinned
by `cmake/Dependencies.cmake` (GLFW 3.4, Dear ImGui v1.92.9b, ImPlot v1.0,
Catch2 v3.7.1) — no system packages beyond a working OpenGL/X11 (Linux)
toolchain. GLFW is built X11-only on Linux (see the Wayland note in
`cmake/Dependencies.cmake`).

ImPlot 1.0 replaced the trailing `flags/offset/stride` params of the `PlotX`
functions with a single `ImPlotSpec`. `ImPlotSpec` is not an aggregate, so build
it with `{ImPlotProp_Flags, value, ...}` prop-pairs or by assigning fields, not
a designated initializer.

```sh
cmake --preset debug          # configure (Ninja + clang/clang++)
cmake --build --preset debug
ctest --preset debug          # run the Catch2 suite
```

- On this Linux box the unversioned `clang++` may be absent; configure with
  `-D CMAKE_CXX_COMPILER=clang++-21` (or set `CC`/`CXX`) instead of the preset.
- `--preset asan` builds with ASan + UBSan.
- Run one test: `./build/debug/tests/vector_math_tests "<test name>"` (Catch2),
  or `ctest --preset debug -R <regex>`.
- `compile_commands.json` is written to the build dir for clang-tidy/editors.
- Headless screenshot: `POLAR_PLOTTER_SCREENSHOT=out.ppm ./build/.../polar-plotter`
  renders a few frames to a hidden window and dumps the framebuffer as a PPM,
  then exits. Under a headless box use `xvfb-run` + `LIBGL_ALWAYS_SOFTWARE=1`;
  convert with `magick out.ppm out.png`.

## Warnings & linting

First-party code is compiled with `-Wall -Wextra -Wpedantic -Werror` plus extras
(see `cmake/CompilerWarnings.cmake`), applied via the `polar_plotter::warnings`
INTERFACE target. Never link it into third-party targets.

- Format: `clang-format-21 -i <files>` (config in `.clang-format`).
- Lint: `clang-tidy-21 -p build <file>` (config in `.clang-tidy`). Only
  `src/`, `app/`, `tests/` are in scope; `build/_deps/**` is excluded.

## Architecture

One-way module dependency graph — do not add edges against it:

| Module (`src/<name>/`) | May depend on | Must NOT depend on |
|---|---|---|
| `vector_math` (`vecmath::`) | standard library only | ImGui, ImPlot, anything UI |
| `polar_plotting` (`polarplot::`) | Dear ImGui, ImPlot | `vector_math`, `ui` |
| `ui` (`ui::`) | `vector_math`, `polar_plotting`, ImGui, ImPlot | GLFW / windowing |
| `app/` | `ui`, plus GLFW/OpenGL host glue (Linux/macOS) and Vulkan-on-Windows host glue (`volk` + Vulkan-Headers, Windows only) | — |

`polar_plotting` is meant to be liftable into another project, so it keeps its
own `polarplot::Point` rather than reaching for `vecmath::Vec2`. Conversions
happen in `ui`.

Each module is `src/<name>/{include/<name>/*.hpp, src/*.cpp}` with its own
`CMakeLists.txt` exporting a `polar_plotter::<name>` alias target.

## Conventions

- TDD: write the failing Catch2 test first, in `tests/`. Tests target public
  module interfaces, not internals.
- Naming (enforced by clang-tidy): `lower_case` functions/variables/namespaces,
  `CamelCase` types, trailing `_` on private members, `k`-prefixed `CamelCase`
  constants.
- No disk persistence beyond `ui::Config` (a flat `key=value` text file for
  last-used inputs). Don't introduce other file I/O.
- `imgui_test_engine` is intentionally not used.

## Portability

Targets Linux, Windows (MSYS2 `clang64`), and macOS, all with Clang. Keep
platform branches in `app/` and the CMake dependency layer; modules stay
platform-agnostic.

Windows renders via Vulkan (`app/vulkan_backend.{hpp,cpp}`), not OpenGL — a
hard cutover with no runtime fallback; see
`docs/adr/0004-windows-vulkan-hard-cutover.md`. Linux and macOS are
unaffected and keep the OpenGL3 + Dear ImGui/ImPlot path in `app/main.cpp`.

## Agent skills

### Issue tracker

Issues live as GitHub issues in `WookieLumberjack/polar-plotter`, via the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Default five canonical labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
