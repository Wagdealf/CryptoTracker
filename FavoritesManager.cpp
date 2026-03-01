/**
 * @file FavoritesManager.cpp
 * @brief Implementation of FavoritesManager
 */

#include "FavoritesManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

FavoritesManager::FavoritesManager(const std::string& filepath)
    : m_filepath(filepath) {}

void FavoritesManager::load()
{
    if (!fs::exists(m_filepath)) return;

    std::ifstream ifs(m_filepath);
    if (!ifs) return;

    try {
        auto j = json::parse(ifs);
        for (const auto& id : j["favorites"])
            m_favorites.insert(id.get<std::string>());
    } catch (...) {}
}

void FavoritesManager::save()
{
    fs::create_directories(fs::path(m_filepath).parent_path());
    std::ofstream ofs(m_filepath);
    if (!ofs) return;

    json j;
    j["favorites"] = json::array();
    for (const auto& id : m_favorites)
        j["favorites"].push_back(id);

    ofs << j.dump(2);
}

bool FavoritesManager::isFavorite(const std::string& id) const
{
    return m_favorites.count(id) > 0;
}

void FavoritesManager::toggle(const std::string& id)
{
    if (m_favorites.count(id))
        m_favorites.erase(id);
    else
        m_favorites.insert(id);
    save();
}
