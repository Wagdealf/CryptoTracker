# CryptoTracker – C++ Advanced Final Project

A **real-time cryptocurrency price tracker** built with:

| Requirement | Implementation |
|---|---|
| STL containers | `std::vector`, `std::unordered_map` (DataManager, AppManager) |
| File I/O | `std::fstream` + `std::filesystem` – saves prices & favorites to disk |
| `std::thread` | Background fetch thread in `DataManager::workerLoop()` |
| `std::atomic` | `m_running`, `m_fetching`, `m_refresh_req` — race-free flags |
| `std::mutex` | `m_data_mutex` (assets), `m_error_mutex` (error string) |
| httplib | HTTPS GET requests to CoinGecko API |
| nlohmann/json | Parse and write JSON responses |
| ImGui | Full GUI: asset list, search, favorites, detail panel |
| ImPlot (bonus) | Interactive price history chart |

---

## Architecture

```
main.cpp
 └── AppManager          ← GLFW + ImGui lifecycle, UI rendering
      ├── DataManager     ← std::thread worker, httplib, mutex/atomic
      │    └── CoinGecko API  (HTTPS JSON)
      └── FavoritesManager ← fstream / filesystem persistence
```

## Prerequisites (Ubuntu/Debian)

```bash
sudo apt install -y cmake git libglfw3-dev libglew-dev libssl-dev
```

## Build

```bash
git clone <your-repo>
cd CryptoTracker
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/CryptoTracker
```

## Features

- **Live prices** – auto-refreshes every 30 seconds via CoinGecko free API
- **Search** – filter coins by name or ticker symbol
- **Favorites** – star coins; persisted across sessions in `data/favorites.json`
- **Price history chart** – 7 / 14 / 30-day interactive chart (ImPlot)
- **Async fetch** – UI never blocks; all HTTP calls run on a background thread
- **Error reporting** – displays last API error in the status bar

## Project Structure

```
CryptoTracker/
├── main.cpp              # Entry point
├── AppManager.h/.cpp     # UI + main loop
├── DataManager.h/.cpp    # Background HTTP + thread safety
├── FavoritesManager.h/.cpp  # Disk persistence
├── CryptoData.h          # Data structures
└── CMakeLists.txt        # Build system
```

## Thread Safety Details

`DataManager` uses two distinct mutexes:

- **`m_data_mutex`** – protects the `m_assets` map (read by UI, written by worker)
- **`m_error_mutex`** – protects `m_last_error` string

The worker thread's lifecycle is controlled by `std::atomic<bool> m_running`,
which avoids race conditions on the control flag without a mutex.
`std::atomic<bool> m_fetching` lets the UI show a loading indicator without locking.

## API Used

[CoinGecko](https://www.coingecko.com/en/api) – free, no API key required.

Endpoint: `GET https://api.coingecko.com/api/v3/coins/markets`
