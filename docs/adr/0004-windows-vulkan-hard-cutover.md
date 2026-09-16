# Switch Windows to Vulkan, hard cutover, Windows-only

On Windows 11, the OpenGL (`imgui_impl_opengl3`) rendering path this app has
used since its first commit stalls/pauses while the window is being
maximized. That gesture is common enough (users routinely maximize a desktop
app on first run) that the stall reads as the app hanging, not a cosmetic
glitch.

## Decision

`app/` gains a Vulkan rendering path, used **exclusively on Windows**
builds. Linux and macOS keep the existing OpenGL (`imgui_impl_opengl3`) path
unchanged — this is not a cross-platform backend replacement, and neither
platform has a reported version of this problem.

The Windows build requires a working Vulkan driver. There is **no runtime
fallback to OpenGL** if Vulkan initialization fails at startup — see
`app/vulkan_backend.cpp`'s `run_vulkan_clear_window()`, which prints an
actionable message to stderr and exits rather than retrying under OpenGL.
This keeps the Windows code path singular instead of doubling the
backends (and the test/support surface) on that one platform.

This first slice (#99) intentionally stops at a bare Vulkan pipeline that
presents a solid clear color — instance, physical/logical device,
swapchain, render pass, framebuffers, command buffers, and sync objects,
with the swapchain recreated correctly across resize/maximize — with no
ImGui/ImPlot rendering on top yet. It exists to validate, as cheaply as
possible, that switching backends actually eliminates the maximize-time
stall before investing in the larger ImGui-over-Vulkan integration (a
sibling ticket under #96).

## Rationale

This exact stall, on this exact toolchain (Clang/MSYS2, GLFW, Dear ImGui,
ImPlot), was previously reproduced and fixed identically on a prior project
by switching that project's Windows build from OpenGL to Vulkan. Nothing
about that fix was toolchain-agnostic guesswork — it was a targeted,
already-verified-effective change applied to the same combination of
libraries this project also uses. Re-deriving the fix from first principles
here (e.g. auditing GLFW's Win32 modal-resize message pump for a smaller,
non-backend-swap root cause) was explicitly decided against in favor of
trusting that directly-applicable prior experience; see the parent
spec's (#96) "Out of Scope" section.

No Vulkan SDK is required to build or run: GLFW dynamically loads the
Vulkan loader (`vulkan-1.dll`) at runtime with no link-time import library,
and pairing it with **volk** (MIT — a small meta-loader that resolves the
remaining instance/device Vulkan function pointers at runtime) needs only
**Vulkan-Headers** (Apache-2.0 OR MIT — header-only type/constant
declarations) for build-time types. Both are fetched and pinned via CMake
`FetchContent` in `cmake/Dependencies.cmake`, the same convention every
other third-party dependency in this project already follows (exact tag,
`GIT_SHALLOW TRUE`). This is the same GLFW+volk pairing Dear ImGui's own
`example_glfw_vulkan` uses.

## Trade-offs accepted

- **Two rendering backends to maintain, split by platform.** `app/` now
  has an OpenGL/ImGui/ImPlot path (Linux/macOS) and a separate Vulkan path
  (Windows), rather than one backend everywhere. A future ImGui/ImPlot
  change that touches rendering has two places to consider, not one — though
  in practice `ui::App`'s render logic is backend-agnostic; only the host
  glue in `app/` forks.
- **No graceful degradation on Windows.** A user whose GPU driver lacks a
  working Vulkan implementation (in practice, essentially only very old or
  broken driver installs — `vulkan-1.dll` support has been standard on
  Windows GPU drivers for roughly eight years) gets a hard launch failure
  with an actionable message, not a degraded-but-working OpenGL session.
  Accepted because doubling the Windows-specific backend/test surface to
  support an edge case with no reported occurrence isn't worth it, and
  because the alternative (silently falling back) would make a future
  Vulkan regression on Windows invisible until a user without OpenGL
  fallback hit it anyway.
- **This ADR's rationale rests on unverifiable-here prior experience.**
  "This was fixed the same way on a prior project" cannot be independently
  checked by a reader of this repository; it is taken on faith from the
  person/team who made this decision, unlike the other trade-offs above
  which are self-contained engineering arguments. This is called out
  explicitly rather than dressed up as a technical inevitability.

## Alternatives considered

- **Debug the OpenGL stall further** (e.g. the Win32 modal-resize message
  pump, `WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE` handling inside GLFW, or
  driver-level swap-interval behavior during a resize). Rejected: this is
  exactly the investigation the prior project already did to arrive at "switch
  to Vulkan," so repeating it here would be re-paying a cost already paid
  once, for the same toolchain.
- **Switch every platform to Vulkan**, not just Windows. Rejected: Linux and
  macOS have no reported version of this problem, so moving them off a
  working, simpler OpenGL path would add backend-maintenance cost with no
  corresponding user-facing benefit — explicitly out of scope per #96.
- **Add a Windows OpenGL→Vulkan runtime fallback** if Vulkan init fails.
  Rejected: doubles the Windows-specific code and test surface for a
  contingency (`vulkan-1.dll` missing) rare enough on today's hardware that
  the parent spec (#96) explicitly accepts the app simply not launching in
  that case instead.
