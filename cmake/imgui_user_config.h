// Project-owned Dear ImGui user config, wired in via IMGUI_USER_CONFIG on the
// `imgui` CMake target (cmake/Dependencies.cmake), so every translation unit
// that touches ImDrawList sees the identical definition -- required for ABI
// consistency, not just a local #define. See #97 (and the parent spec #96).
//
// Dear ImGui defaults ImDrawIdx to 16-bit. ImPlot's own documentation warns
// that a draw list needing more than 65,536 indices can silently corrupt
// rendering with that default. This project doesn't hit that ceiling today,
// but nothing had ever explicitly decided against the risk -- widen to
// 32-bit up front.
#pragma once

// Must be a macro, not a using-alias/typedef: imgui.h guards its own
// `typedef unsigned short ImDrawIdx;` with `#ifndef ImDrawIdx`, so only a
// preprocessor definition suppresses the default.
#define ImDrawIdx unsigned int
