#pragma once

#include "client/tank_interface.hpp"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace dandy::client {

// Loaded plugin handle
class LoadedPlugin {
public:
    LoadedPlugin(void* handle, CreateTankAI create, DestroyTankAI destroy, PluginInfo info);
    ~LoadedPlugin();

    LoadedPlugin(const LoadedPlugin&) = delete;
    LoadedPlugin& operator=(const LoadedPlugin&) = delete;
    LoadedPlugin(LoadedPlugin&& other) noexcept;
    LoadedPlugin& operator=(LoadedPlugin&& other) noexcept;

    [[nodiscard]] ITankAI* create_ai(ITankController* controller) const;
    void destroy_ai(ITankAI* ai) const;
    [[nodiscard]] const PluginInfo& info() const { return info_; }

private:
    void* handle_{nullptr};
    CreateTankAI create_fn_{nullptr};
    DestroyTankAI destroy_fn_{nullptr};
    PluginInfo info_;
};

// Plugin loader
class PluginLoader {
public:
    PluginLoader() = default;
    ~PluginLoader() = default;

    // Load a plugin from a .so file
    [[nodiscard]] std::unique_ptr<LoadedPlugin> load(const std::filesystem::path& path);

    // Scan directory for plugins
    [[nodiscard]] std::vector<std::filesystem::path> scan_directory(
        const std::filesystem::path& dir);

    // Get last error
    [[nodiscard]] const std::string& last_error() const { return last_error_; }

private:
    std::string last_error_;
};

} // namespace dandy::client
