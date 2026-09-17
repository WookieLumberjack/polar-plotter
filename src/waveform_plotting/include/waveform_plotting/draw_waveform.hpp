#ifndef WAVEFORM_PLOTTING_DRAW_WAVEFORM_HPP
#define WAVEFORM_PLOTTING_DRAW_WAVEFORM_HPP

/// \file
/// The drawing entry point for the time-domain waveform plot (#105/#108),
/// mirroring polar_plotting::draw_scene's shape: a stateless function taking
/// buffer contents plus display parameters, with no ownership of app state.
/// Depends only on Dear ImGui and ImPlot -- deliberately no dependency on
/// polar_plotting (a peer module, not a base) or ui, so this stays liftable
/// into another project exactly like polar_plotting.

#include <vector>

#include "waveform_plotting/waveform_buffer.hpp"

namespace waveform_plotting {

/// RGBA color for one trace (components in [0, 1]). Kept as a plain struct,
/// like polar_plotting::MarkerColor, rather than reaching for that type --
/// waveform_plotting must not depend on polar_plotting (see CLAUDE.md's
/// module table) even though the two happen to share this shape, so each
/// module keeps its own self-contained copy.
struct TraceColor {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

/// One trace to plot: \p buffer's contents, drawn as \p label in \p color.
/// \p buffer is a non-owning pointer -- the buffers themselves are owned by
/// the caller (ui::App, one per named-vector slot) and outlive this call.
struct Trace {
    const char* label{nullptr};
    const WaveformBuffer* buffer{nullptr};
    TraceColor color;
};

/// Draws the entire waveform plot body for one frame -- ImPlot::BeginPlot
/// through ImPlot::EndPlot -- from \p traces: each trace's buffer is unrolled
/// from its ring-buffer cursor (oldest sample first) into a fixed 5 second
/// x-axis span in seconds, right edge = "now" (x = 0), scrolling left as time
/// passes (see WaveformBuffer::kSampleCount / kSampleRateHz). The vertical
/// axis always auto-fits the currently-plotted traces' contents (ImPlot's own
/// auto-fit, re-evaluated fresh every frame) -- there is no manual-override
/// control for it, matching the design's "no plot-limits management" intent.
/// A trace with a null \p buffer is skipped. No-op (draws an empty plot) when
/// \p traces is empty. Callers decide \p traces' membership -- e.g. only the
/// currently-shown named vectors, mirroring polar_plotting::PlotPlan's own
/// show/hide gating -- this function has no notion of visibility beyond "is
/// it in the list".
void draw_waveform_plot(const char* title, const std::vector<Trace>& traces);

}  // namespace waveform_plotting

#endif  // WAVEFORM_PLOTTING_DRAW_WAVEFORM_HPP
