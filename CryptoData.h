/**
 * @file CryptoData.h
 * @brief Data structures for cryptocurrency information
 */

#pragma once
#include <string>
#include <vector>

/**
 * @struct PricePoint
 * @brief Represents a single historical price point
 */
struct PricePoint {
    double timestamp; ///< Unix timestamp
    double price;     ///< Price in USD
};

/**
 * @struct CryptoAsset
 * @brief Full information about a cryptocurrency asset
 */
struct CryptoAsset {
    std::string id;             ///< CoinGecko API ID (e.g., "bitcoin")
    std::string name;           ///< Display name (e.g., "Bitcoin")
    std::string symbol;         ///< Ticker symbol (e.g., "BTC")
    double      price_usd;      ///< Current price in USD
    double      change_24h;     ///< 24-hour percentage change
    double      market_cap;     ///< Market capitalization in USD
    double      volume_24h;     ///< 24-hour trading volume in USD
    std::vector<PricePoint> history; ///< Historical price data
    bool        is_favorite;    ///< Whether user marked as favorite
};
