/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "BagReader.hpp"
#include "TimingData.hpp"

// Per-topic display state
struct TopicDisplayState
{
  bool  visible  = true;
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  // X data for scatter (time in seconds from bag start)
  std::vector<double> xs;
  // Y data for scatter (constant = topic index, used as lane)
  std::vector<double> ys;
};

class BagInspectorApp
{
   public:
    BagInspectorApp();
    ~BagInspectorApp();

    // Kick off an async bag load.  Returns immediately; progress shown in GUI.
    void load_bag(const std::string& path);

    // Render one frame -- call inside the ImGui NewFrame / Render pair.
    void render();

    bool wants_quit() const { return m_wants_quit; }

   private:
    void render_timeline_tab();
    void render_histograms_tab();
    void render_menu_bar();
    void render_loading_modal();
    void rebuild_plot_data();
    void finish_load();  // called once async thread is done

    BagTimingData m_data;
    std::vector<TopicDisplayState> m_topic_display;

    // Async loading
    std::thread                    m_load_thread;
    std::unique_ptr<BagTimingData> m_pending_data;
    std::unique_ptr<LoadProgress>  m_load_progress;

    // Measure tool state (timeline tab)
    bool   m_measure_active     = false;
    double m_measure_t1         = 0.0;
    double m_measure_t2         = 0.0;
    bool   m_measure_has_second = false;

    // Error / status message shown in a modal
    std::string m_status_message;
    bool        m_show_status_modal = false;

    bool m_wants_quit = false;
};
