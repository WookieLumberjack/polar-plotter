#include "waveform_plotting/draw_waveform.hpp"

#include <array>
#include <cstddef>

#include <imgui.h>
#include <implot.h>

namespace waveform_plotting {
namespace {

// Fixed span (seconds) of the x axis -- exactly the buffer's own window
// (kSampleCount samples at kSampleRateHz), so the ruler and the plotted data
// can never disagree.
constexpr double kWindowSeconds =
    static_cast<double>(WaveformBuffer::kSampleCount) / static_cast<double>(kSampleRateHz);

// x[k] for k in [0, kSampleCount): oldest sample (k = 0) sits at the left
// edge (-kWindowSeconds, plus one sample period so the *newest* sample lands
// exactly at 0 = "now"), newest sample (k = kSampleCount - 1) sits at x = 0.
std::array<double, WaveformBuffer::kSampleCount> waveform_x_axis() {
    std::array<double, WaveformBuffer::kSampleCount> xs{};
    for (std::size_t k = 0; k < WaveformBuffer::kSampleCount; ++k) {
        xs[k] = (static_cast<double>(k) - static_cast<double>(WaveformBuffer::kSampleCount - 1)) /
                static_cast<double>(kSampleRateHz);
    }
    return xs;
}

}  // namespace

void draw_waveform_plot(const char* title, const std::vector<Trace>& traces) {
    constexpr ImPlotFlags kPlotFlags = ImPlotFlags_NoInputs;
    if (!ImPlot::BeginPlot(title, ImVec2(-1, -1), kPlotFlags)) {
        return;
    }

    // X1: fixed 5 second span, right edge always "now" -- never user-panned
    // or zoomed (ImPlotFlags_NoInputs above). Y1: always auto-fit to
    // whatever's currently plotted -- no manual-override control exists.
    constexpr ImPlotAxisFlags kTimeAxisFlags = ImPlotAxisFlags_None;
    constexpr ImPlotAxisFlags kAmplitudeAxisFlags = ImPlotAxisFlags_AutoFit;
    ImPlot::SetupAxes("Time (s)", "Amplitude", kTimeAxisFlags, kAmplitudeAxisFlags);
    ImPlot::SetupAxisLimits(ImAxis_X1, -kWindowSeconds, 0.0, ImPlotCond_Always);

    static const std::array<double, WaveformBuffer::kSampleCount> kXs = waveform_x_axis();
    std::array<double, WaveformBuffer::kSampleCount> ys{};

    for (const Trace& trace : traces) {
        if (trace.buffer == nullptr) {
            continue;
        }
        for (std::size_t k = 0; k < WaveformBuffer::kSampleCount; ++k) {
            const std::size_t idx = (trace.buffer->cursor + k) % WaveformBuffer::kSampleCount;
            ys[k] = static_cast<double>(trace.buffer->samples[idx]);
        }
        ImPlotSpec spec;
        spec.LineColor = ImVec4(trace.color.r, trace.color.g, trace.color.b, trace.color.a);
        ImPlot::PlotLine(trace.label, kXs.data(), ys.data(),
                         static_cast<int>(WaveformBuffer::kSampleCount), spec);
    }

    ImPlot::EndPlot();
}

}  // namespace waveform_plotting
