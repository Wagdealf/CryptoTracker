/**
 * @file FavoritesManager.h
 * @brief Persists user's favorite coins to disk using fstream + filesystem
 */

#pragma once
#include <string>
#include <vector>
#include <unordered_set>

/**
 * @class FavoritesManager
 * @brief Loads and saves the set of favorite coin IDs to a JSON file
 */
class FavoritesManager {
public:
    explicit FavoritesManager(const std::string& filepath = "data/favorites.json");

    void load();  ///< Load favorites from disk
    void save();  ///< Save favorites to disk

    bool isFavorite(const std::string& id) const;
    void toggle(const std::string& id);
    const std::unordered_set<std::string>& getFavorites() const { return m_favorites; }

private:
    std::string                     m_filepath;
    std::unordered_set<std::string> m_favorites;
};
