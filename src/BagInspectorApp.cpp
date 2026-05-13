/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#include "BagInspectorApp.hpp"
#include "BagReader.hpp"

#include <imgui.h>
#include <implot.h>
#include <tinyfiledialogs.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

// ----------------------------------------------------------------------------
// Palette: visually distinct colors for up to ~20 topics
// ----------------------------------------------------------------------------
static const float kPalette[][3] = {
    {0.122f, 0.467f, 0.706f}, {1.000f, 0.498f, 0.055f},
    {0.173f, 0.627f, 0.173f}, {0.839f, 0.153f, 0.157f},
    {0.580f, 0.404f, 0.741f}, {0.549f, 0.337f, 0.294f},
    {0.890f, 0.467f, 0.761f}, {0.498f, 0.498f, 0.498f},
    {0.737f, 0.741f, 0.133f}, {0.090f, 0.745f, 0.812f},
    {0.682f, 0.780f, 0.910f}, {1.000f, 0.733f, 0.471f},
    {0.596f, 0.875f, 0.541f}, {1.000f, 0.596f, 0.588f},
    {0.773f, 0.690f, 0.835f}, {0.729f, 0.537f, 0.514f},
    {0.969f, 0.714f, 0.824f}, {0.780f, 0.780f, 0.780f},
    {0.859f, 0.859f, 0.553f}, {0.620f, 0.855f, 0.898f},
};
static constexpr int kPaletteSize = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));

// ----------------------------------------------------------------------------

BagInspectorApp::BagInspectorApp() = default;

BagInspectorApp::~BagInspectorApp()
{
  if (m_load_thread.joinable())
  {
    m_load_thread.join();
  }
}

// ----------------------------------------------------------------------------

void BagInspectorApp::load_bag(const std::string& path)
{
  // If a previous load is still running, wait for it first.
  if (m_load_thread.joinable())
  {
    m_load_thread.join();
    }

    m_pending_data       = std::make_unique<BagTimingData>();
    m_load_progress      = std::make_unique<LoadProgress>();
    m_measure_active     = false;
    m_measure_has_second = false;

    m_load_thread =
        std::thread(read_bag_timing_async, path, m_pending_data.get(), m_load_progress.get());
}

void BagInspectorApp::finish_load()
{
  if (m_load_thread.joinable())
  {
    m_load_thread.join();
  }

  if (m_load_progress->failed.load())
  {
    m_status_message    = std::string("Failed to load bag:\n") + m_load_progress->error_message;
    m_show_status_modal = true;
  }
  else
  {
    m_data = std::move(*m_pending_data);
    rebuild_plot_data();
  }

  m_pending_data.reset();
  m_load_progress.reset();
}

void BagInspectorApp::rebuild_plot_data()
{
    m_topic_display.clear();
    m_topic_display.resize(m_data.topics.size());

    for (size_t i = 0; i < m_data.topics.size(); i++)
    {
      auto&       tds = m_topic_display[i];
      const auto& ttd = m_data.topics[i];

      const int ci = static_cast<int>(i) % kPaletteSize;
      tds.color[0] = kPalette[ci][0];
      tds.color[1] = kPalette[ci][1];
      tds.color[2] = kPalette[ci][2];
      tds.color[3] = 1.0f;
      tds.visible  = true;

      const size_t n = ttd.timestamps_ns.size();
      tds.xs.resize(n);
      tds.ys.resize(n);
      const double lane = static_cast<double>(i);
      for (size_t j = 0; j < n; j++)
      {
        tds.xs[j] = static_cast<double>(ttd.timestamps_ns[j] - m_data.start_time_ns) * 1e-9;
        tds.ys[j] = lane;
        }
    }
}

// ----------------------------------------------------------------------------
// Loading modal
// ----------------------------------------------------------------------------
void BagInspectorApp::render_loading_modal()
{
  if (m_load_progress == nullptr)
  {
    return;
  }

  // Open the modal on the first frame it appears
  ImGui::OpenPopup("Loading##modal");

  const ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(
      ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
      ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Always);

  if (ImGui::BeginPopupModal(
          "Loading##modal", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
  {
    const uint64_t read  = m_load_progress->messages_read.load();
    const uint64_t total = m_load_progress->messages_total.load();

    ImGui::Text("Loading bag, please wait...");

    if (total > 0)
    {
      const float fraction = static_cast<float>(read) / static_cast<float>(total);
      char        overlay[64];
      std::snprintf(
          overlay, sizeof(overlay), "%llu / %llu msgs", static_cast<unsigned long long>(read),
          static_cast<unsigned long long>(total));
      ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
    }
    else
    {
      // Total unknown -- show a spinner-like marquee bar
      const float t     = static_cast<float>(ImGui::GetTime());
      const float phase = std::fmod(t * 0.5f, 1.0f);
      ImGui::ProgressBar(phase, ImVec2(-1, 0), "Reading...");
    }

    // Check if done
    if (m_load_progress->done.load())
    {
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
      finish_load();
      return;
    }

    ImGui::EndPopup();
  }
}

// ----------------------------------------------------------------------------
// Menu bar
// ----------------------------------------------------------------------------
void BagInspectorApp::render_menu_bar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open bag...", "Ctrl+O"))
            {
                const char* filters[] = {"*.mcap", "*.db3"};
                const char* path      = tinyfd_openFileDialog(
                    "Open ROS2 bag", "", 2, filters, "ROS2 bag files", 0);
                if (path != nullptr)
                {
                    load_bag(path);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                m_wants_quit = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

// ----------------------------------------------------------------------------
// Timeline tab
// ----------------------------------------------------------------------------
void BagInspectorApp::render_timeline_tab()
{
    const size_t n_topics = m_data.topics.size();

    // Left panel: topic list with visibility toggles
    ImGui::BeginChild("topic_list", ImVec2(220, 0), ImGuiChildFlags_Border);
    ImGui::Text("Topics (%zu)", n_topics);
    ImGui::Separator();

    if (ImGui::SmallButton("All"))
    {
      for (auto& tds : m_topic_display)
      {
        tds.visible = true;
      }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("None"))
    {
      for (auto& tds : m_topic_display)
      {
        tds.visible = false;
      }
    }
    ImGui::Separator();

    for (size_t i = 0; i < n_topics; i++)
    {
      auto&       tds = m_topic_display[i];
      const auto& ttd = m_data.topics[i];

      ImGui::PushID(static_cast<int>(i));

      ImGui::ColorEdit4(
          "##col", tds.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      ImGui::SameLine();
      ImGui::Checkbox(ttd.topic_name.c_str(), &tds.visible);

      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Type: %s", ttd.message_type.c_str());
        ImGui::Text("Messages: %zu", ttd.timestamps_ns.size());
        if (ttd.mean_frequency_hz > 0.0)
        {
          ImGui::Text("Mean freq: %.2f Hz", ttd.mean_frequency_hz);
        }
        ImGui::EndTooltip();
        }

        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel: plot
    ImGui::BeginGroup();

    // Measure tool toggle
    ImGui::Checkbox("Measure mode", &m_measure_active);
    if (m_measure_active)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(click once for T1, again for T2)");
        if (m_measure_has_second)
        {
            const double dt = m_measure_t2 - m_measure_t1;
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "  DeltaT = %.6f s  (%.3f ms)", dt, dt * 1e3);
        }
        else
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1), "  T1 not set");
        }
    }

    const double bag_dur = m_data.duration_s();
    const double y_max   = static_cast<double>(n_topics);

    if (ImPlot::BeginPlot("##timeline", ImVec2(-1, -1), ImPlotFlags_NoLegend))
    {
        ImPlot::SetupAxes(
            "Time [s]", "Topic",
            ImPlotAxisFlags_None,
            ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, bag_dur > 0 ? bag_dur : 1.0, ImGuiCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, y_max - 0.5, ImGuiCond_Once);

        // Custom Y tick labels = topic names
        {
          std::vector<double>      y_ticks(n_topics);
          std::vector<const char*> y_labels(n_topics);
          for (size_t i = 0; i < n_topics; i++)
          {
            y_ticks[i]  = static_cast<double>(i);
            y_labels[i] = m_data.topics[i].topic_name.c_str();
            }
            ImPlot::SetupAxisTicks(
                ImAxis_Y1, y_ticks.data(), static_cast<int>(n_topics), y_labels.data());
        }

        // Draw each topic's messages as scatter points
        for (size_t i = 0; i < n_topics; i++)
        {
            auto& tds = m_topic_display[i];
            if (!tds.visible || tds.xs.empty())
            {
                continue;
            }

            ImPlot::SetNextMarkerStyle(
                ImPlotMarker_Circle, 3.0f,
                ImVec4(tds.color[0], tds.color[1], tds.color[2], tds.color[3]),
                IMPLOT_AUTO,
                ImVec4(tds.color[0], tds.color[1], tds.color[2], tds.color[3]));

            ImPlot::PlotScatter(
                m_data.topics[i].topic_name.c_str(),
                tds.xs.data(), tds.ys.data(),
                static_cast<int>(tds.xs.size()));
        }

        // Topic-lane tooltip: show topic name when hovering within 0.4 lanes of a row
        if (ImPlot::IsPlotHovered())
        {
          const double mouse_y = ImPlot::GetPlotMousePos().y;
          const int    lane    = static_cast<int>(std::round(mouse_y));
          if (lane >= 0 && lane < static_cast<int>(n_topics) &&
              std::fabs(mouse_y - static_cast<double>(lane)) < 0.4)
          {
            const auto& ttd = m_data.topics[static_cast<size_t>(lane)];
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(ttd.topic_name.c_str());
            ImGui::TextDisabled("%s", ttd.message_type.c_str());
            ImGui::Text("%zu msgs", ttd.timestamps_ns.size());
            if (ttd.mean_frequency_hz > 0.0)
            {
              ImGui::Text("%.2f Hz", ttd.mean_frequency_hz);
            }
            ImGui::EndTooltip();
          }
        }

        // Measure vertical lines
        if (m_measure_active)
        {
            if (m_measure_has_second || m_measure_t1 != 0.0)
            {
                ImPlot::DragLineX(
                    0, &m_measure_t1, ImVec4(1, 1, 0, 1), 1.5f,
                    ImPlotDragToolFlags_NoFit);
            }
            if (m_measure_has_second)
            {
                ImPlot::DragLineX(
                    1, &m_measure_t2, ImVec4(0, 1, 1, 1), 1.5f,
                    ImPlotDragToolFlags_NoFit);
            }

            // Click handler: set T1 then T2
            if (ImPlot::IsPlotHovered() &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGui::GetIO().KeyCtrl)
            {
                const double clicked_t = ImPlot::GetPlotMousePos().x;
                if (!m_measure_has_second && m_measure_t1 == 0.0)
                {
                    m_measure_t1 = clicked_t;
                }
                else if (!m_measure_has_second)
                {
                    m_measure_t2         = clicked_t;
                    m_measure_has_second = true;
                }
                else
                {
                    // Reset and start over
                    m_measure_t1         = clicked_t;
                    m_measure_t2         = 0.0;
                    m_measure_has_second = false;
                }
            }
        }

        ImPlot::EndPlot();
    }

    ImGui::EndGroup();
}

// ----------------------------------------------------------------------------
// Histograms tab
// ----------------------------------------------------------------------------
void BagInspectorApp::render_histograms_tab()
{
    const size_t n_topics = m_data.topics.size();
    if (n_topics == 0)
    {
        ImGui::TextDisabled("No bag loaded.");
        return;
    }

    ImGui::Columns(2, "hist_cols", true);

    ImGui::Text("Inter-message interval histograms");
    ImGui::Separator();

    const float plot_h = std::max(
        100.0f,
        (ImGui::GetContentRegionAvail().y - 30.0f) / static_cast<float>(n_topics));

    for (size_t i = 0; i < n_topics; i++)
    {
        const auto& ttd = m_data.topics[i];
        auto&       tds = m_topic_display[i];

        if (ttd.intervals_s.empty())
        {
            continue;
        }

        char label[256];
        std::snprintf(label, sizeof(label), "##hist_%zu", i);

        if (ImPlot::BeginPlot(label, ImVec2(-1, plot_h)))
        {
            ImPlot::SetupAxes("Interval [s]", "Count");
            ImPlot::SetNextFillStyle(
                ImVec4(tds.color[0], tds.color[1], tds.color[2], 0.7f));
            ImPlot::PlotHistogram(
                ttd.topic_name.c_str(), ttd.intervals_s.data(),
                static_cast<int>(ttd.intervals_s.size()), ImPlotBin_Sqrt, 1.0,
                ImPlotRange(ttd.min_interval_s, ttd.max_interval_s));
            ImPlot::EndPlot();
        }
    }

    ImGui::NextColumn();

    ImGui::Text("Timing statistics");
    ImGui::Separator();

    if (ImGui::BeginTable(
            "stats_table", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Topic");
        ImGui::TableSetupColumn("Count");
        ImGui::TableSetupColumn("Mean [ms]");
        ImGui::TableSetupColumn("Std [ms]");
        ImGui::TableSetupColumn("Min [ms]");
        ImGui::TableSetupColumn("Max [ms]");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < n_topics; i++)
        {
            const auto& ttd = m_data.topics[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(ttd.topic_name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", ttd.timestamps_ns.size());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", ttd.mean_interval_s * 1e3);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", ttd.std_interval_s * 1e3);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", ttd.min_interval_s * 1e3);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.3f", ttd.max_interval_s * 1e3);
        }
        ImGui::EndTable();
    }

    ImGui::Columns(1);
}

// ----------------------------------------------------------------------------
// Main render
// ----------------------------------------------------------------------------
void BagInspectorApp::render()
{
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);

  const ImGuiWindowFlags main_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("##main_window", nullptr, main_flags);
  render_menu_bar();

  // Show loading modal if a load is in progress
  const bool is_loading = (m_load_progress != nullptr);
  if (is_loading)
  {
    render_loading_modal();
    ImGui::End();
    return;
  }

  if (m_data.empty())
  {
    ImGui::TextDisabled(
        "No bag loaded. Use File > Open bag... or pass a path on the command line.");
  }
  else
  {
    ImGui::Text(
        "Bag: %s   |   Duration: %.3f s   |   Topics: %zu", m_data.bag_path.c_str(),
        m_data.duration_s(), m_data.topics.size());
    ImGui::Separator();

    if (ImGui::BeginTabBar("main_tabs"))
    {
      if (ImGui::BeginTabItem("Timeline"))
      {
        render_timeline_tab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Histograms / Stats"))
      {
        render_histograms_tab();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }

  ImGui::End();

  // Error modal
  if (m_show_status_modal)
  {
    ImGui::OpenPopup("Error##modal");
    m_show_status_modal = false;
  }
  if (ImGui::BeginPopupModal("Error##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_status_message.c_str());
    ImGui::Separator();
    if (ImGui::Button("OK", ImVec2(120, 0)))
    {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
