/**
 * @file DataManager.cpp
 * @brief Implementation of DataManager – background HTTP fetching
 */

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "DataManager.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
DataManager::DataManager(std::vector<std::string> coin_ids, int interval_sec)
    : m_coin_ids(std::move(coin_ids))
    , m_interval_sec(interval_sec)
    , m_running(false)
    , m_fetching(false)
    , m_refresh_req(false)
{
    // Pre-populate asset map with display metadata
    static const std::unordered_map<std::string, std::pair<std::string,std::string>> meta = {
        {"bitcoin",  {"Bitcoin",  "BTC"}},
        {"ethereum", {"Ethereum", "ETH"}},
        {"solana",   {"Solana",   "SOL"}},
        {"ripple",   {"XRP",      "XRP"}},
        {"cardano",  {"Cardano",  "ADA"}},
        {"dogecoin", {"Dogecoin", "DOGE"}},
    };

    for (const auto& id : m_coin_ids) {
        CryptoAsset asset{};
        asset.id          = id;
        asset.is_favorite = false;
        if (auto it = meta.find(id); it != meta.end()) {
            asset.name   = it->second.first;
            asset.symbol = it->second.second;
        } else {
            asset.name   = id;
            asset.symbol = id;
        }
        m_assets[id] = asset;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
DataManager::~DataManager()
{
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::start()
{
    if (m_running.load()) return;
    m_running.store(true);
    m_worker = std::thread(&DataManager::workerLoop, this);
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::stop()
{
    m_running.store(false);
    if (m_worker.joinable())
        m_worker.join();
}

// ─────────────────────────────────────────────────────────────────────────────
std::unordered_map<std::string, CryptoAsset> DataManager::getAssets() const
{
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_assets; // Return a copy for thread-safe access
}

// ─────────────────────────────────────────────────────────────────────────────
std::string DataManager::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_error_mutex);
    return m_last_error;
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::requestRefresh()
{
    m_refresh_req.store(true);
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::workerLoop()
{
    // Fetch immediately on start
    fetchCurrentPrices();

    while (m_running.load()) {
        // Sleep in short increments so we can respond to stop/refresh requests
        for (int i = 0; i < m_interval_sec * 10 && m_running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (m_refresh_req.exchange(false)) break; // Early refresh requested
        }
        if (!m_running.load()) break;
        fetchCurrentPrices();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::fetchCurrentPrices()
{
    m_fetching.store(true);

    // Build comma-separated coin list for query
    std::string ids;
    for (size_t i = 0; i < m_coin_ids.size(); ++i) {
        if (i) ids += ',';
        ids += m_coin_ids[i];
    }

    // CoinGecko free API – no key required
    httplib::SSLClient cli("api.coingecko.com");
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);

    std::string path = "/api/v3/coins/markets"
                       "?vs_currency=usd"
                       "&ids=" + ids +
                       "&order=market_cap_desc"
                       "&sparkline=false"
                       "&price_change_percentage=24h";

    auto res = cli.Get(path.c_str());

    if (!res || res->status != 200) {
        std::lock_guard<std::mutex> lock(m_error_mutex);
        m_last_error = res ? "HTTP error: " + std::to_string(res->status)
                           : "Connection failed";
        m_fetching.store(false);
        return;
    }

    try {
        auto j = json::parse(res->body);

        std::lock_guard<std::mutex> lock(m_data_mutex);
        for (const auto& item : j) {
            std::string id = item.value("id", "");
            if (m_assets.find(id) == m_assets.end()) continue;

            auto& asset       = m_assets[id];
            asset.price_usd   = item.value("current_price",          0.0);
            asset.change_24h  = item.value("price_change_percentage_24h", 0.0);
            asset.market_cap  = item.value("market_cap",             0.0);
            asset.volume_24h  = item.value("total_volume",           0.0);
            asset.name        = item.value("name",   asset.name);
            asset.symbol      = item.value("symbol", asset.symbol);
        }

        // Persist latest prices to disk using fstream + filesystem
        fs::create_directories("data");
        std::ofstream ofs("data/latest_prices.json");
        if (ofs) ofs << j.dump(2);

        {
            std::lock_guard<std::mutex> elock(m_error_mutex);
            m_last_error.clear();
        }

    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(m_error_mutex);
        m_last_error = std::string("JSON parse error: ") + e.what();
    }

    m_fetching.store(false);
}

// ─────────────────────────────────────────────────────────────────────────────
void DataManager::fetchHistory(const std::string& coin_id, int days)
{
    // Spawn a one-shot thread so the UI doesn't block
    std::thread([this, coin_id, days]() {
        httplib::SSLClient cli("api.coingecko.com");
        cli.set_connection_timeout(10);
        cli.set_read_timeout(15);

        std::string path = "/api/v3/coins/" + coin_id +
                           "/market_chart?vs_currency=usd&days=" +
                           std::to_string(days) + "&interval=daily";

        auto res = cli.Get(path.c_str());
        if (!res || res->status != 200) return;

        try {
            auto j = json::parse(res->body);
            std::vector<PricePoint> pts;
            for (const auto& pt : j["prices"]) {
                PricePoint p;
                p.timestamp = pt[0].get<double>() / 1000.0; // ms → s
                p.price     = pt[1].get<double>();
                pts.push_back(p);
            }
            std::lock_guard<std::mutex> lock(m_data_mutex);
            if (m_assets.count(coin_id))
                m_assets[coin_id].history = std::move(pts);
        } catch (...) {}
    }).detach(); // Detached – safe because DataManager outlives UI loop
}
