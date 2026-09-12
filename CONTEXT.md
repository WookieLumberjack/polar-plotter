# polar-plotter

An interactive desktop tool for learning 2D vector math by plotting named
vectors on a polar canvas. The domain is deliberately generic — vector
geometry and plot presentation — not any specific engineering discipline;
polar-plot conventions researched from rotor-diagnostics sources (see
`research/references.md`) inform the *visual design* only, and their
domain-specific vocabulary (sensors, probes, Keyphasor, balance weights)
does not enter this glossary.

## Language

**Named vector**:
A vector with a label the UI displays and lets the user reason about — `A`,
`B`, and derived vectors like `A + B`, `A − B`, `B − A`, `A × B` (complex
product), `A ÷ B`, or `B ÷ A` (complex quotient). Every named vector gets a
tip marker and a tip label when drawn.
_Avoid_: quantity, input vector (for A/B specifically — they're named vectors
like any other, just user-editable ones)

**Complex product / Complex quotient**:
`A × B` and `A ÷ B` (and `B ÷ A`): derived vectors computed by treating each
named vector's Cartesian components as a complex number (Real, Imaginary)
and multiplying/dividing under complex-number arithmetic — amplitude
multiplies/divides, phase adds/subtracts. Distinct from the existing scalar
dot product (`A . B`) shown elsewhere in the UI, which is unrelated. Amplitude
and Phase are the same underlying magnitude/angle already used elsewhere,
just labeled to match the Real/Imag pairing.

**Angle convention**:
The pair of settings (zero direction, angle sign) that maps a vector's
math-convention angle to where it's actually drawn on a polar plot. Owned by
`polar_plotting`; carries no domain semantics beyond "where is 0° and which
way do angles increase."
_Avoid_: display convention

**Zero direction**:
Where 0° is drawn on the plot, as an offset from a fixed reference direction
(plot-up). One half of an angle convention.

**Angle sign**:
Whether increasing angle is drawn clockwise or counterclockwise on the plot.
The other half of an angle convention. Computed by `ui` as the composition of
rotation direction and measurement convention; `polar_plotting` only ever
sees the single resulting sign, never the two inputs that produced it.

**Rotation direction**:
A `ui`-level toggle (CW/CCW) — one of two user-facing inputs that compose
into an angle sign. Not known to `polar_plotting`.

**Measurement convention**:
A `ui`-level toggle for whether a plotted angle increases with rotation
("with rotation") or against it ("against rotation") — the second
user-facing input that composes into an angle sign, alongside rotation
direction: `angle sign = rotation direction × measurement convention`. Not
known to `polar_plotting`.

**Tip marker**:
A small dot or cross-hair `polar_plotting` draws at a named vector's tip.
Style (dot vs. cross-hair) is a single setting shared by every named vector,
not chosen per vector. Always present, unconditionally — including at zero
length, where it sits at the origin — so it never appears/disappears based
on the vector's size; that consistency is the point.

**Tip label**:
Text drawn adjacent to a tip marker, naming the vector it belongs to (just
the name, e.g. `A` — not its magnitude/angle, which already lives in the
side panel). General mechanism for every named vector, not a special case
for zero or near-zero results.

**Annotation**:
Something `ui` passes to `polar_plotting` purely to be drawn — `polar_plotting`
has no concept of what it represents (a construction line, a sum, a
reference mark, whatever) or why it's being shown. This keeps composition
("what extra things does a construction display need") a `ui`-level
decision; `polar_plotting` only ever draws annotations it's handed.
Two kinds:
- **Annotation vector**: drawn as an arrow, either from the origin or as a
  free vector positioned wherever `ui` places it (e.g. one endpoint at
  another vector's tip).
- **Angle arc**: drawn between the plot's zero direction and a given angle.
  Two independent lifecycles, both driven by `ui`: *transient* (shown while
  an angle-valued input is actively being edited, to make its meaning
  visually unambiguous) and *persistent* (kept on the plot until the user
  removes it, e.g. to document a specific angle for a screenshot).
_Avoid_: construction line (a `ui`-level *use* of an annotation vector, not
a `polar_plotting` concept)

**Tip-to-tail construction**:
A `ui`-level annotation-vector display showing how a sum/difference is
built: for `A + B`, a copy of `B` from `A`'s tip (and, for the full
parallelogram, a copy of `A` from `B`'s tip) both arriving at the resultant;
for `A − B`, a copy of `-B` from `A`'s tip arriving at the resultant.
Independently toggleable from the difference segment below.

**Difference segment**:
A `ui`-level annotation-vector display, meaningful only for `A − B`: the
free vector connecting `B`'s tip to `A`'s tip — congruent to `A − B` itself,
but drawn where the two source vectors actually are rather than at the
origin, as a visual proof that the two are the same vector. Independently
toggleable from the tip-to-tail construction.
