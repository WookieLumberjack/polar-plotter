# Pin Dear ImGui to the `docking` branch, not master

Real docking (panels the user can drag into a dockspace, split, and
re-arrange, persisted across restarts) requires Dear ImGui's `docking`
branch — `ImGuiConfigFlags_DockingEnable`, `ImGui::DockSpace()`, and the
`DockBuilder` API used to lay out the default split do not exist on `master`.
There is no way to get this feature (#57/#58's whole point) without moving
off the `v1.92.9b` tag this repo previously pinned.

We pin to **`v1.92.9b-docking`** (commit `b48d1afbe8ee8b238e2961dc363a949dd7304e23`,
2026-07-31) rather than the moving tip of `docking` or a bare SHA further
along it. ocornut maintains `docking` as `master` plus the docking/viewports
feature set, and cuts a matching `-docking` tag for every release tag
(confirmed back through `v1.92.2-docking` … `v1.92.9b-docking`). Picking the
docking tag with the *same version number* as the `v1.92.9b` tag this repo
already builds cleanly against (with the pinned ImPlot `v1.0`) is the
lowest-risk entry point: identical widget/table/draw-list API surface to
what's already verified working, with docking layered on top and no
unrelated master-branch changes riding along. It is still a real upgrade,
not a no-op — `git log v1.92.9b..v1.92.9b-docking` shows roughly 1800 commits
of accumulated docking-branch history merged in.

Because `v1.92.9b-docking` is a real, published tag (not a bare commit SHA),
`GIT_SHALLOW TRUE` in `cmake/Dependencies.cmake` continues to work exactly as
it does today: shallow fetches work against advertised refs (branches and
tags), but generally cannot fetch an arbitrary unadvertised SHA — so pinning
to a tag rather than a raw SHA on `docking` avoids having to either drop
`GIT_SHALLOW` (slower full clone) or otherwise special-case the fetch.

## Trade-offs accepted

- **Moving off a stable release tag onto a maintained side branch.**
  `docking` is not merged into `master` and, per upstream, is not planned to
  be — it is ocornut's long-term-supported integration branch, but it is
  still a second branch to track rather than the single mainline. Future
  ImGui version bumps in this repo must remember to pick the `-docking`
  variant of whatever tag they'd otherwise use, or silently fall back to a
  non-docking build.
- **No official ImPlot variant for the docking branch.** ImPlot is written
  against the public ImGui API surface (draw lists, tables, IO, widgets) and
  is reported to work against `docking` in practice, but there is no ImPlot
  release or branch that states docking-branch compatibility as a supported
  configuration. This repo's own build + test suite is the compatibility
  check, not an upstream guarantee.
- **Hard to reverse cheaply.** Once dockspace/`DockBuilder` code exists in
  `ui`/`app`, reverting to a non-docking ImGui tag would require removing
  that code, not just repointing the tag — this is a one-way door in
  practice, which is why it gets an ADR rather than being folded silently
  into the feature commit.

## Alternatives considered

- **Stay on `master`/`v1.92.9b`, roll a hand-built dockspace-like layout.**
  Rejected: reimplementing drag-to-dock, splits, and persistence outside
  ImGui's own mechanism is substantially more code and would still not match
  the acceptance criteria's use of `ImGui::DockSpace()` /
  `ImGui::DockBuilderGetNode()` / `imgui.ini`-backed persistence.
- **Pin to `docking`'s branch tip (latest commit) instead of a tagged
  commit.** Rejected: no shallow-fetch guarantee for an arbitrary SHA (see
  above), and a moving tip is a worse reproducibility story than a tagged
  commit that will never change underneath us.
