/**
 * @file AppManager.h
 * @brief High-level application controller
 *
 * Initialises GLFW + ImGui, owns the DataManager and FavoritesManager,
 * and drives the main render loop.
 */

#pragma once

#include "DataManager.h"
#include "FavoritesManager.h"
#include <string>
#include <vector>

struct GLFWwindow; // Forward declaration to avoid GLFW header pollution

/**
 * @class AppManager
 * @brief Top-level orchestrator: window, ImGui, data, and UI
 */
class AppManager {
public:
    AppManager();
    ~AppManager();

    bool init();   ///< Set up GLFW, OpenGL, ImGui, DataManager
    void run();    ///< Main loop
    void cleanup();

private:
    // ── UI Rendering helpers ──────────────────────────────────────────────
    void renderMainWindow();
    void renderAssetList(const std::unordered_map<std::string, CryptoAsset>& assets);
    void renderDetailPanel(const CryptoAsset& asset);
    void renderPriceChart(const CryptoAsset& asset);
    void renderStatusBar();

    // ── Utilities ─────────────────────────────────────────────────────────
    static std::string formatUSD(double value);
    static std::string formatPct(double value);

    GLFWwindow*      m_window     = nullptr;
    DataManager*     m_data_mgr   = nullptr;
    FavoritesManager m_favorites;

    std::string      m_search_buf;
    std::string      m_selected_id;   ///< Currently selected asset ID
    bool             m_show_favorites = false;
    int              m_history_days   = 7;
};
