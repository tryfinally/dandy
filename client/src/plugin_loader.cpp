#include "client/plugin_loader.hpp"
#include <dlfcn.h>
#include <iostream>

namespace dandy::client {

LoadedPlugin::LoadedPlugin(void* handle, CreateTankAI create, DestroyTankAI destroy, PluginInfo info)
    : handle_(handle)
    , create_fn_(create)
    , destroy_fn_(destroy)
    , info_(std::move(info))
{
}

LoadedPlugin::~LoadedPlugin() {
    if (handle_) {
        dlclose(handle_);
    }
}

LoadedPlugin::LoadedPlugin(LoadedPlugin&& other) noexcept
    : handle_(other.handle_)
    , create_fn_(other.create_fn_)
    , destroy_fn_(other.destroy_fn_)
    , info_(std::move(other.info_))
{
    other.handle_ = nullptr;
    other.create_fn_ = nullptr;
    other.destroy_fn_ = nullptr;
}

LoadedPlugin& LoadedPlugin::operator=(LoadedPlugin&& other) noexcept {
    if (this != &other) {
        if (handle_) {
            dlclose(handle_);
        }
        handle_ = other.handle_;
        create_fn_ = other.create_fn_;
        destroy_fn_ = other.destroy_fn_;
        info_ = std::move(other.info_);
        other.handle_ = nullptr;
        other.create_fn_ = nullptr;
        other.destroy_fn_ = nullptr;
    }
    return *this;
}

ITankAI* LoadedPlugin::create_ai(ITankController* controller) const {
    if (create_fn_) {
        return create_fn_(controller);
    }
    return nullptr;
}

void LoadedPlugin::destroy_ai(ITankAI* ai) const {
    if (destroy_fn_ && ai) {
        destroy_fn_(ai);
    }
}

std::unique_ptr<LoadedPlugin> PluginLoader::load(const std::filesystem::path& path) {
    last_error_.clear();

    // Open the shared library
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        last_error_ = dlerror();
        std::cerr << "Failed to load plugin: " << last_error_ << std::endl;
        return nullptr;
    }

    // Get required symbols
    auto create_fn = reinterpret_cast<CreateTankAI>(dlsym(handle, "create_tank_ai"));
    if (!create_fn) {
        last_error_ = "Missing create_tank_ai symbol";
        dlclose(handle);
        return nullptr;
    }

    auto destroy_fn = reinterpret_cast<DestroyTankAI>(dlsym(handle, "destroy_tank_ai"));
    if (!destroy_fn) {
        last_error_ = "Missing destroy_tank_ai symbol";
        dlclose(handle);
        return nullptr;
    }

    // Get optional info function
    PluginInfo info;
    auto info_fn = reinterpret_cast<GetPluginInfo>(dlsym(handle, "get_plugin_info"));
    if (info_fn) {
        info = info_fn();
    } else {
        info.name = path.stem().string();
        info.version = "unknown";
        info.author = "unknown";
        info.description = "";
    }

    std::cout << "Loaded plugin: " << info.name << " v" << info.version << std::endl;
    if (!info.description.empty()) {
        std::cout << "  " << info.description << std::endl;
    }

    return std::make_unique<LoadedPlugin>(handle, create_fn, destroy_fn, std::move(info));
}

std::vector<std::filesystem::path> PluginLoader::scan_directory(
    const std::filesystem::path& dir)
{
    std::vector<std::filesystem::path> plugins;

    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return plugins;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".so" || ext == ".dylib") {
                plugins.push_back(entry.path());
            }
        }
    }

    return plugins;
}

} // namespace dandy::client
