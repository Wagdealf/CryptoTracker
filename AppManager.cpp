/**
 * @file AppManager.cpp
 * @brief Full implementation of AppManager – ImGui UI + application logic
 */

#include "AppManager.h"

// OpenGL / GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Coin IDs to track (CoinGecko IDs)
static const std::vector<std::string> TRACKED_COINS = {
    "bitcoin", "ethereum", "solana", "ripple", "cardano", "dogecoin"
};

// ─────────────────────────────────────────────────────────────────────────────
AppManager::AppManager()
    : m_favorites("data/favorites.json")
{}

AppManager::~AppManager()
{
    cleanup();
}

// ─────────────────────────────────────────────────────────────────────────────
bool AppManager::init()
{
    // ── GLFW ─────────────────────────────────────────────────────────────
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(1280, 720, "CryptoTracker – C++ Advanced Project", nullptr, nullptr);
    if (!m_window) { glfwTerminate(); return false; }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync

    if (glewInit() != GLEW_OK) return false;

    // ── ImGui ─────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 6.0f;
    style.FrameRounding    = 4.0f;
    style.ItemSpacing      = ImVec2(8, 6);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ── Data Layer ────────────────────────────────────────────────────────
    m_favorites.load();
    m_data_mgr = new DataManager(TRACKED_COINS, 30);
    m_data_mgr->start();

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::run()
{
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMainWindow();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::cleanup()
{
    if (m_data_mgr) {
        m_data_mgr->stop();
        delete m_data_mgr;
        m_data_mgr = nullptr;
    }
    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::renderMainWindow()
{
    ImGuiIO& io = ImGui::GetIO();

    // Full-screen dockspace window
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##root", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // ── Title bar ────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.76f, 0.0f, 1.0f));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("  CryptoTracker");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 30);
    ImGui::TextDisabled("Live prices via CoinGecko API");

    ImGui::Separator();

    // ── Toolbar ──────────────────────────────────────────────────────────
    // Search box
    char search[128] = {};
    std::copy(m_search_buf.begin(),
              m_search_buf.begin() + std::min(m_search_buf.size(), (size_t)127),
              search);
    ImGui::SetNextItemWidth(220);
    if (ImGui::InputTextWithHint("##search", "Search coins...", search, sizeof(search)))
        m_search_buf = search;

    ImGui::SameLine();
    if (ImGui::Button(m_show_favorites ? "Show All" : "Show Favorites"))
        m_show_favorites = !m_show_favorites;

    ImGui::SameLine();
    if (m_data_mgr->isFetching()) {
        ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Fetching...");
    } else {
        if (ImGui::Button("Refresh Now"))
            m_data_mgr->requestRefresh();
    }

    ImGui::Separator();

    // ── Two-column layout: list | detail ─────────────────────────────────
    auto assets = m_data_mgr->getAssets();  // Thread-safe snapshot

    float list_width = 340.0f;
    ImGui::BeginChild("##list", {list_width, -28}, true);
    renderAssetList(assets);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##detail", {0, -28}, true);
    if (!m_selected_id.empty() && assets.count(m_selected_id))
        renderDetailPanel(assets.at(m_selected_id));
    else
        ImGui::TextDisabled("← Select a coin to view details");
    ImGui::EndChild();

    renderStatusBar();
    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::renderAssetList(const std::unordered_map<std::string, CryptoAsset>& assets)
{
    ImGui::Text("%-12s %10s  %8s", "Name", "Price (USD)", "24h %");
    ImGui::Separator();

    // Collect and optionally sort/filter
    std::vector<const CryptoAsset*> sorted;
    sorted.reserve(assets.size());
    for (const auto& [id, asset] : assets) {
        if (m_show_favorites && !m_favorites.isFavorite(id)) continue;
        if (!m_search_buf.empty()) {
            std::string lower_name = asset.name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            std::string lower_sym  = asset.symbol;
            std::transform(lower_sym.begin(), lower_sym.end(), lower_sym.begin(), ::tolower);
            std::string q = m_search_buf;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            if (lower_name.find(q) == std::string::npos &&
                lower_sym.find(q)  == std::string::npos) continue;
        }
        sorted.push_back(&asset);
    }
    std::sort(sorted.begin(), sorted.end(), [](const CryptoAsset* a, const CryptoAsset* b){
        return a->market_cap > b->market_cap;
    });

    for (const auto* asset : sorted) {
        bool selected = (asset->id == m_selected_id);

        // Color the change column
        ImVec4 chg_color = asset->change_24h >= 0
            ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f)
            : ImVec4(0.95f, 0.3f, 0.3f, 1.0f);

        // Favorite indicator
        std::string fav_icon = m_favorites.isFavorite(asset->id) ? "★ " : "  ";

        std::string label = fav_icon + asset->name + " (" + asset->symbol + ")";
        if (ImGui::Selectable(label.c_str(), selected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
            m_selected_id = asset->id;
            m_data_mgr->fetchHistory(asset->id, m_history_days);
        }

        ImGui::SameLine(180);
        ImGui::Text("%s", formatUSD(asset->price_usd).c_str());

        ImGui::SameLine(270);
        ImGui::TextColored(chg_color, "%s", formatPct(asset->change_24h).c_str());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::renderDetailPanel(const CryptoAsset& asset)
{
    // ── Header ───────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.76f, 0.0f, 1.0f));
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("%s  (%s)", asset.name.c_str(), asset.symbol.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 20);
    std::string fav_label = m_favorites.isFavorite(asset.id)
        ? "★ Remove Favorite" : "☆ Add Favorite";
    if (ImGui::Button(fav_label.c_str())) {
        m_favorites.toggle(asset.id);
    }

    ImGui::Separator();

    // ── Key metrics ──────────────────────────────────────────────────────
    ImVec4 chg_color = asset.change_24h >= 0
        ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f)
        : ImVec4(0.95f, 0.3f, 0.3f, 1.0f);

    ImGui::Text("Current Price:");
    ImGui::SameLine(160);
    ImGui::Text("%s", formatUSD(asset.price_usd).c_str());

    ImGui::Text("24h Change:");
    ImGui::SameLine(160);
    ImGui::TextColored(chg_color, "%s", formatPct(asset.change_24h).c_str());

    ImGui::Text("Market Cap:");
    ImGui::SameLine(160);
    ImGui::Text("%s", formatUSD(asset.market_cap).c_str());

    ImGui::Text("24h Volume:");
    ImGui::SameLine(160);
    ImGui::Text("%s", formatUSD(asset.volume_24h).c_str());

    ImGui::Separator();

    // ── History controls ──────────────────────────────────────────────────
    ImGui::Text("Chart range:");
    ImGui::SameLine();
    const char* ranges[] = {"7 days", "14 days", "30 days"};
    const int   days[]   = {7, 14, 30};
    for (int i = 0; i < 3; ++i) {
        if (i) ImGui::SameLine();
        bool active = (m_history_days == days[i]);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.6f,1.0f,1.0f));
        if (ImGui::Button(ranges[i])) {
            m_history_days = days[i];
            m_data_mgr->fetchHistory(asset.id, m_history_days);
        }
        if (active) ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // ── Price chart ───────────────────────────────────────────────────────
    renderPriceChart(asset);
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::renderPriceChart(const CryptoAsset& asset)
{
    if (asset.history.empty()) {
        ImGui::TextDisabled("Loading price history...");
        return;
    }

    // Build arrays for ImPlot
    std::vector<double> xs, ys;
    xs.reserve(asset.history.size());
    ys.reserve(asset.history.size());
    for (const auto& pt : asset.history) {
        xs.push_back(pt.timestamp);
        ys.push_back(pt.price);
    }

    // Axis formatting callback using a lambda passed as function ptr
    auto fmt_date = [](double val, char* buf, int size, void*) -> int {
        time_t t = (time_t)val;
        struct tm* tm_info = gmtime(&t);
        return strftime(buf, size, "%b %d", tm_info);
    };

    if (ImPlot::BeginPlot(("##chart_" + asset.id).c_str(),
                          ImVec2(-1, 300),
                          ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes("Date", "Price (USD)");
        ImPlot::SetupAxisFormat(ImAxis_X1, fmt_date, nullptr);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

        // Shade area under curve
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.2f, 0.6f, 1.0f, 0.2f));
        ImPlot::PlotShaded("", xs.data(), ys.data(), (int)xs.size(), -INFINITY);
        ImPlot::PopStyleColor();

        // Line
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        ImPlot::SetNextLineStyle(IMPLOT_AUTO_COL, 2.0f);
        ImPlot::PlotLine("", xs.data(), ys.data(), (int)xs.size());
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void AppManager::renderStatusBar()
{
    std::string err = m_data_mgr->getLastError();
    if (!err.empty()) {
        ImGui::TextColored({1.0f, 0.4f, 0.3f, 1.0f}, "Error: %s", err.c_str());
    } else {
        ImGui::TextDisabled("Data refreshes every 30 seconds. Powered by CoinGecko API.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
std::string AppManager::formatUSD(double value)
{
    std::ostringstream oss;
    if (value >= 1e9)
        oss << "$" << std::fixed << std::setprecision(2) << value / 1e9 << "B";
    else if (value >= 1e6)
        oss << "$" << std::fixed << std::setprecision(2) << value / 1e6 << "M";
    else if (value >= 1.0)
        oss << "$" << std::fixed << std::setprecision(2) << value;
    else
        oss << "$" << std::fixed << std::setprecision(6) << value;
    return oss.str();
}

std::string AppManager::formatPct(double value)
{
    std::ostringstream oss;
    oss << (value >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << value << "%";
    return oss.str();
}
