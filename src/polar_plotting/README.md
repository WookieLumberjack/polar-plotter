# polar_plotting

Thin helpers (`polarplot::`) for drawing vectors on an ImPlot canvas with a
polar reference grid. This module depends only on Dear ImGui and ImPlot —
deliberately not on `vector_math` or `ui` — so it can be lifted into another
project by copying `include/polar_plotting/` and `src/` wholesale. See the
file-header doc comment in
[`polar_plot.hpp`](include/polar_plotting/polar_plot.hpp) for the same
statement in code.

## Per-frame lifecycle

The module's one real entry point is `draw_scene`: given a `PlotFrame`, a
`PlotPlan` (what to draw this frame — A, B, whichever derived vectors and
annotations are currently toggled on), and a `DrawStyle` (presentation
choices: marker style, line width, per-label color lookup), it draws the
entire polar plot body for the frame and returns an optional `SceneResult`
(A/B's post-interaction state, for a caller that wants to react to a drag).

```cpp
const std::optional<polarplot::SceneResult> result =
    polarplot::draw_scene(view_frame, plan, style, a_dragging, b_dragging);
```

`draw_scene` itself spans `begin_vector_plot` through `end_vector_plot`
internally — including the "call `end_vector_plot` iff `begin_vector_plot`
returned `true`" contract `ImPlot::BeginPlot`/`EndPlot` already has (ImPlot
may cull an off-screen or collapsed plot and skip its internal setup), so a
`draw_scene` caller never has to reason about that itself. A caller composes
`PlotPlan`/`DrawStyle` fresh each frame from whatever toggles/state it owns
(e.g. `ui::plan_plot`) and re-issues the single `draw_scene` call every
frame, the same way it would with ImPlot's own per-frame draw calls — none
of `polar_plotting`'s state persists across frames on its own.

`begin_vector_plot`, `end_vector_plot`, `draw_polar_grid`,
`draw_rotation_indicator`, `draw_vector`, `draw_annotation_vector`,
`draw_angle_arc`, and the rest of the individual "draw this now" primitives
`draw_scene` composes are still public — mainly so they can be exercised as
independently-tested pure/near-pure seams (see `tests/polar_plot_tests.cpp`)
and so a caller with an unusual per-frame composition need isn't forced
through `draw_scene`'s fixed sequencing. A caller assembling its own
`begin_vector_plot`/.../`end_vector_plot` sequence by hand, instead of going
through `draw_scene`, is still responsible for the same
`begin_vector_plot`/`end_vector_plot` pairing contract and for driving every
call from the same `PlotFrame` value for that frame, so the scale ruler,
grid rings, and rotation indicator's radius can't disagree.

## What a caller composes and supplies each frame

`polar_plotting` draws geometry; it does not decide what that geometry means.
`PlotPlan` and `DrawStyle` (the two values `draw_scene` takes) are themselves
built from two smaller, domain-free value types that carry every per-frame
decision a caller must have already made before calling in:

- **`AngleConvention`** — where 0 is drawn (`zero_direction`, radians, in the
  plot's own coordinate frame) and which way angle increases as it grows
  (`angle_sign`, +1 or −1). A caller composes whatever domain-specific inputs
  it has (e.g. a rotation-direction toggle and a measurement convention) into
  this single resolved pair before handing it to `apply_angle_convention`,
  `to_plotted_point`, `draw_arrow`, `draw_vector`, `draw_annotation_vector`,
  or `draw_polar_grid`. See
  `docs/adr/0001-polar-plotting-receives-only-composed-angle-sign.md` for why
  the composition happens on the caller's side of this boundary.
- **`PlotFrame`** — the three related numbers describing one frame's polar
  view: the plotted axis extent (`extent()`, applied as ± on both axes), the
  radius spacing between rings (`ring_interval()`), and how many rings are
  drawn (`ring_count()`). Constructed via `PlotFrame(ring_interval,
  ring_count)`, which asserts both are strictly positive; `extent()` is
  derived from them on every call rather than stored, so `extent() ==
  ring_interval() * ring_count()` holds by construction — there is no second
  stored number for it to disagree with. A caller (e.g. `ui::plot_plan`) is
  responsible for deciding `ring_interval`/`ring_count` together and handing
  them to the constructor, rather than deriving `extent` independently at
  each call site — that's what lets `begin_vector_plot`'s scale ruler,
  `draw_polar_grid`'s rings, and `draw_rotation_indicator`'s radius agree by
  construction instead of by convention. `inflate_for_labels(PlotFrame)` is a
  pure helper that computes the extra axis headroom `begin_vector_plot` needs
  so spoke-degree labels drawn just outside the outer ring aren't clipped;
  callers never derive or pass that inflated value themselves.

Both types are small values owned entirely by this module — a caller composes
them fresh each frame (or reuses a previous frame's values when nothing
changed) and passes them into the relevant calls above.

## What this module does not do

`polar_plotting` is intentionally shallow in scope, which is what keeps it
liftable into another project by file copy:

- **No domain vocabulary.** It has no concept of "rotation direction,"
  "measurement convention," "named vector" (as a domain idea — it has
  `draw_vector` for the *mechanical* act of drawing a labelled, marked
  arrow), or any other term belonging to this app's learner-facing glossary
  in `CONTEXT.md`. Every parameter it takes is a plain geometric or visual
  value (points, angles, radii, colors, line weights) or one of this
  module's own domain-free types (`AngleConvention`, `PlotFrame`,
  `ArcStyle`, `TipMarkerStyle`). Callers translate their own domain
  vocabulary into these plain values before calling in.
- **No ImGui window management.** It draws *into* whatever ImPlot plot is
  currently open; it never creates, sizes, positions, or docks an ImGui
  window. Hosting a plot inside a window is entirely the caller's concern.
- **No persistence.** Nothing here reads or writes a file, a config, or any
  other durable state. Every value it draws is supplied fresh by the caller
  each frame.

This keeps the module a pure "draw vectors and a polar grid" library:
everything about *why* a vector is named, *why* an angle is measured a
certain way, or *what* the surrounding UI looks like stays outside it.
