/**
 * @file DataManager.h
 * @brief Manages background data fetching from CoinGecko API
 *
 * Uses std::thread and std::atomic to fetch cryptocurrency data
 * asynchronously without blocking the UI thread.
 */

#pragma once

#include "CryptoData.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

/**
 * @class DataManager
 * @brief Fetches and stores cryptocurrency data in the background
 *
 * Spawns a worker thread that periodically polls the CoinGecko API.
 * Thread-safe access to shared data is ensured via std::mutex.
 * The running flag uses std::atomic<bool> to avoid race conditions.
 */
class DataManager {
public:
    /**
     * @brief Construct with a list of coin IDs to track
     * @param coin_ids  List of CoinGecko coin IDs (e.g., {"bitcoin","ethereum"})
     * @param interval_sec  Polling interval in seconds (default 30)
     */
    explicit DataManager(std::vector<std::string> coin_ids, int interval_sec = 30);

    /// Destructor – stops background thread cleanly
    ~DataManager();

    /// Start background fetching thread
    void start();

    /// Stop background fetching thread (blocks until thread joins)
    void stop();

    /**
     * @brief Get a snapshot of all tracked assets (thread-safe)
     * @return Copy of the current asset map
     */
    std::unordered_map<std::string, CryptoAsset> getAssets() const;

    /**
     * @brief Fetch historical price data for a specific coin (thread-safe)
     * @param coin_id  CoinGecko coin ID
     * @param days     Number of past days to retrieve
     */
    void fetchHistory(const std::string& coin_id, int days = 7);

    /// Returns true if a fetch is currently in progress
    bool isFetching() const { return m_fetching.load(); }

    /// Returns the last error message, empty string if none
    std::string getLastError() const;

    /// Force an immediate refresh (non-blocking, signals worker thread)
    void requestRefresh();

private:
    /// Worker thread function – loops until m_running is false
    void workerLoop();

    /// Fetch current prices from CoinGecko (called from worker thread)
    void fetchCurrentPrices();

    std::vector<std::string>                        m_coin_ids;
    int                                             m_interval_sec;

    mutable std::mutex                              m_data_mutex;    ///< Guards m_assets
    std::unordered_map<std::string, CryptoAsset>    m_assets;        ///< Shared data

    std::atomic<bool>                               m_running;       ///< Thread control flag
    std::atomic<bool>                               m_fetching;      ///< True during HTTP call
    std::atomic<bool>                               m_refresh_req;   ///< Immediate refresh flag

    mutable std::mutex                              m_error_mutex;   ///< Guards m_last_error
    std::string                                     m_last_error;

    std::thread                                     m_worker;        ///< Background thread
};
