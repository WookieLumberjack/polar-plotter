# polar_plotting

Thin helpers (`polarplot::`) for drawing vectors on an ImPlot canvas with a
polar reference grid. This module depends only on Dear ImGui and ImPlot —
deliberately not on `vector_math` or `ui` — so it can be lifted into another
project by copying `include/polar_plotting/` and `src/` wholesale. See the
file-header doc comment in
[`polar_plot.hpp`](include/polar_plotting/polar_plot.hpp) for the same
statement in code.

## Per-frame lifecycle

`polar_plotting` mirrors ImPlot's own `BeginPlot`/`EndPlot` pattern: every
frame, a caller opens a plot, issues zero or more draw calls into it, then
closes it.

```cpp
if (polarplot::begin_vector_plot("Vectors", frame /* ...or its constituent fields */)) {
    polarplot::draw_polar_grid(frame, convention);
    polarplot::draw_rotation_indicator(frame, sweep_sign);
    polarplot::draw_vector("A", head, convention, polarplot::TipMarkerStyle::kDot);
    // ...further draw_arrow / draw_vector / draw_annotation_vector /
    // draw_angle_arc calls...

    polarplot::end_vector_plot();
}
```

(Exactly how each entry point consumes `PlotFrame` — the whole struct, or a
destructured field or two — is left to that function's own signature; what's
fixed is that all three entry points below are driven from the *same*
`PlotFrame` value for a given frame, so they can never disagree about
extent, ring interval, or ring count.)

- `begin_vector_plot` returns `true` when the plot is visible. Call
  `end_vector_plot` exactly once **iff** `begin_vector_plot` returned `true`
  — exactly the contract `ImPlot::BeginPlot`/`EndPlot` already has, and for
  the same reason (ImPlot may cull an off-screen or collapsed plot and skip
  its internal setup).
- Everything drawn between the two calls — grid, rotation indicator, named
  vectors, annotation vectors, angle arcs — is a "draw this now" primitive.
  None of them retain state across frames or decide on their own whether
  they should render this frame; the caller re-issues every draw call, every
  frame, for whatever should currently be visible.
- `begin_vector_plot`, `draw_polar_grid`, and `draw_rotation_indicator` are
  three independent entry points, not one composite call: the rotation
  indicator's per-frame draw-or-not decision is separate from the grid's, so
  a caller may open a plot and draw a grid without a rotation indicator, or
  vice versa.

## What a caller composes and supplies each frame

`polar_plotting` draws geometry; it does not decide what that geometry means.
Two small, domain-free value types carry every per-frame decision a caller
must have already made before calling in:

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
  view: the plotted axis extent (`extent`, applied as ± on both axes), the
  radius spacing between rings (`ring_interval`), and how many rings are
  drawn (`ring_count`), always related by `extent == ring_interval *
  ring_count`. A caller (e.g. `ui::plot_plan`) is responsible for deciding
  these three numbers together and handing in one finished bundle, rather
  than deriving them independently at each call site — that's what lets
  `begin_vector_plot`'s scale ruler, `draw_polar_grid`'s rings, and
  `draw_rotation_indicator`'s radius agree by construction instead of by
  convention. `inflate_for_labels(PlotFrame)` is a pure helper that computes
  the extra axis headroom `begin_vector_plot` needs so spoke-degree labels
  drawn just outside the outer ring aren't clipped; callers never derive or
  pass that inflated value themselves.

Both types are plain data owned entirely by this module — a caller composes
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
