#include "plugins/plugin_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <dlfcn.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

namespace netlogai::plugins {

// PluginSandbox Implementation
PluginSandbox::PluginSandbox(const SecurityPolicy& policy)
    : policy_(policy), monitoring_active_(false), current_memory_usage_(0) {}

PluginSandbox::~PluginSandbox() {
    if (monitoring_active_) {
        monitoring_active_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

bool PluginSandbox::initialize() {
    start_time_ = std::chrono::steady_clock::now();

    if (policy_.max_memory_mb > 0 || policy_.max_execution_time_ms > 0) {
        monitoring_active_ = true;
        monitor_thread_ = std::thread([this] { monitor_resource_usage(); });
    }

    return apply_restrictions();
}

bool PluginSandbox::apply_restrictions() {
    // Platform-specific security restrictions would be implemented here
    // For now, we'll implement basic monitoring

#ifdef _WIN32
    // Windows-specific restrictions (process limits, file system ACLs, etc.)
    // This is a simplified implementation
    return true;
#else
    // Unix-specific restrictions (chroot, seccomp, cgroups, etc.)
    if (policy_.max_memory_mb > 0) {
        struct rlimit memory_limit;
        memory_limit.rlim_cur = static_cast<rlim_t>(policy_.max_memory_mb) * 1024 * 1024;
        memory_limit.rlim_max = memory_limit.rlim_cur;

        if (setrlimit(RLIMIT_AS, &memory_limit) != 0) {
            std::cerr << "Warning: Failed to set memory limit for plugin sandbox\n";
        }
    }
    return true;
#endif
}

bool PluginSandbox::monitor_resource_usage() {
    while (monitoring_active_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);

        // Check execution time limit
        if (policy_.max_execution_time_ms > 0 &&
            elapsed.count() > static_cast<long long>(policy_.max_execution_time_ms)) {
            std::cerr << "Plugin execution time exceeded limit: " << elapsed.count() << "ms\n";
            terminate_if_exceeded();
            return false;
        }

        // Check memory usage
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            current_memory_usage_ = static_cast<uint32_t>(pmc.WorkingSetSize / 1024 / 1024);
        }
#else
        // Linux memory monitoring
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.find("VmRSS:") == 0) {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                current_memory_usage_ = static_cast<uint32_t>(std::stoi(value) / 1024);
                break;
            }
        }
#endif

        if (policy_.max_memory_mb > 0 && current_memory_usage_ > policy_.max_memory_mb) {
            std::cerr << "Plugin memory usage exceeded limit: " << current_memory_usage_ << "MB\n";
            terminate_if_exceeded();
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

void PluginSandbox::terminate_if_exceeded() {
    monitoring_active_ = false;
    // In a real implementation, this would forcefully terminate the plugin
    // For now, we just log the violation
    std::cerr << "Plugin sandbox violation detected - plugin should be terminated\n";
}

// PluginLoader Implementation
PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    for (auto& [id, plugin] : loaded_plugins_) {
        if (plugin->handle) {
            unload_dynamic_library(plugin->handle);
        }
    }
}

bool PluginLoader::load_plugin(const std::string& plugin_path) {
    std::lock_guard<std::mutex> lock(loader_mutex_);

    if (!std::filesystem::exists(plugin_path)) {
        std::cerr << "Plugin file not found: " << plugin_path << std::endl;
        return false;
    }

    // Check for manifest file
    std::filesystem::path manifest_path = std::filesystem::path(plugin_path).parent_path() / "plugin.json";
    if (!std::filesystem::exists(manifest_path)) {
        std::cerr << "Plugin manifest not found: " << manifest_path << std::endl;
        return false;
    }

    // Parse manifest
    PluginManifest manifest = parse_plugin_manifest(manifest_path.string());
    if (manifest.name.empty()) {
        std::cerr << "Invalid plugin manifest: " << manifest_path << std::endl;
        return false;
    }

    // Check if already loaded
    if (loaded_plugins_.find(manifest.name) != loaded_plugins_.end()) {
        std::cerr << "Plugin already loaded: " << manifest.name << std::endl;
        return false;
    }

    // Load dynamic library
    void* handle = load_dynamic_library(plugin_path);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << plugin_path << std::endl;
        return false;
    }

    // Extract plugin functions
    CreatePluginFunc create_func;
    DestroyPluginFunc destroy_func;
    if (!extract_plugin_functions(handle, create_func, destroy_func)) {
        std::cerr << "Failed to extract plugin functions from: " << plugin_path << std::endl;
        unload_dynamic_library(handle);
        return false;
    }

    // Create plugin instance
    std::unique_ptr<INetLogAIPlugin> instance = create_func();
    if (!instance) {
        std::cerr << "Failed to create plugin instance: " << manifest.name << std::endl;
        unload_dynamic_library(handle);
        return false;
    }

    // Verify plugin API compatibility
    if (instance->get_api_version() != NETLOGAI_PLUGIN_API_VERSION) {
        std::cerr << "Plugin API version mismatch: " << manifest.name
                  << " (expected: " << NETLOGAI_PLUGIN_API_VERSION
                  << ", got: " << instance->get_api_version() << ")" << std::endl;
        unload_dynamic_library(handle);
        return false;
    }

    // Create loaded plugin entry
    auto loaded_plugin = std::make_unique<LoadedPlugin>();
    loaded_plugin->path = plugin_path;
    loaded_plugin->handle = handle;
    loaded_plugin->manifest = manifest;
    loaded_plugin->create_func = create_func;
    loaded_plugin->destroy_func = destroy_func;
    loaded_plugin->instance = std::move(instance);
    loaded_plugin->loaded_at = std::chrono::system_clock::now();
    loaded_plugin->is_active = false;

    loaded_plugins_[manifest.name] = std::move(loaded_plugin);

    std::cout << "Successfully loaded plugin: " << manifest.name
              << " v" << manifest.version << std::endl;
    return true;
}

bool PluginLoader::unload_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(loader_mutex_);

    auto it = loaded_plugins_.find(plugin_id);
    if (it == loaded_plugins_.end()) {
        return false;
    }

    auto& plugin = it->second;

    // Stop plugin if active
    if (plugin->is_active && plugin->instance) {
        plugin->instance->stop();
        plugin->instance->cleanup();
    }

    // Unload dynamic library
    if (plugin->handle) {
        unload_dynamic_library(plugin->handle);
    }

    loaded_plugins_.erase(it);
    std::cout << "Unloaded plugin: " << plugin_id << std::endl;
    return true;
}

PluginManifest PluginLoader::parse_plugin_manifest(const std::string& manifest_path) {
    PluginManifest manifest;

    try {
        std::ifstream file(manifest_path);
        if (!file.is_open()) {
            return manifest;
        }

        json j;
        file >> j;

        manifest.name = j.value("name", "");
        manifest.version = j.value("version", "");
        manifest.description = j.value("description", "");
        manifest.author = j.value("author", "");
        manifest.api_version = j.value("api_version", "");
        manifest.plugin_type = j.value("plugin_type", "");
        manifest.entry_point = j.value("entry_point", "");

        if (j.contains("capabilities") && j["capabilities"].is_array()) {
            for (const auto& cap : j["capabilities"]) {
                manifest.capabilities.push_back(cap.get<std::string>());
            }
        }

        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            for (const auto& dep : j["dependencies"]) {
                manifest.dependencies.push_back(dep.get<std::string>());
            }
        }

        if (j.contains("metadata") && j["metadata"].is_object()) {
            for (auto& [key, value] : j["metadata"].items()) {
                manifest.metadata[key] = value.get<std::string>();
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing plugin manifest: " << e.what() << std::endl;
        manifest = PluginManifest{}; // Reset to empty
    }

    return manifest;
}

void* PluginLoader::load_dynamic_library(const std::string& path) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path.c_str());
    return static_cast<void*>(handle);
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void PluginLoader::unload_dynamic_library(void* handle) {
    if (!handle) return;

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

bool PluginLoader::extract_plugin_functions(void* handle, CreatePluginFunc& create, DestroyPluginFunc& destroy) {
    if (!handle) return false;

#ifdef _WIN32
    create = reinterpret_cast<CreatePluginFunc>(GetProcAddress(static_cast<HMODULE>(handle), "create_plugin"));
    destroy = reinterpret_cast<DestroyPluginFunc>(GetProcAddress(static_cast<HMODULE>(handle), "destroy_plugin"));
#else
    create = reinterpret_cast<CreatePluginFunc>(dlsym(handle, "create_plugin"));
    destroy = reinterpret_cast<DestroyPluginFunc>(dlsym(handle, "destroy_plugin"));
#endif

    return create != nullptr && destroy != nullptr;
}

PluginLoader::LoadedPlugin* PluginLoader::get_loaded_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    auto it = loaded_plugins_.find(plugin_id);
    return (it != loaded_plugins_.end()) ? it->second.get() : nullptr;
}

std::vector<PluginLoader::LoadedPlugin*> PluginLoader::get_all_loaded_plugins() {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    std::vector<LoadedPlugin*> plugins;
    for (auto& [id, plugin] : loaded_plugins_) {
        plugins.push_back(plugin.get());
    }
    return plugins;
}

// PluginManager Implementation
PluginManager::PluginManager() : real_time_active_(false) {
    loader_ = std::make_unique<PluginLoader>();
}

PluginManager::~PluginManager() {
    shutdown();
}

bool PluginManager::initialize(const PluginConfig& config) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    config_ = config;

    if (config_.auto_load_plugins) {
        return scan_and_load_plugins();
    }

    return true;
}

void PluginManager::shutdown() {
    stop_real_time_processing();

    std::lock_guard<std::mutex> lock(manager_mutex_);

    // Stop and cleanup all plugins
    auto loaded_plugins = loader_->get_all_loaded_plugins();
    for (auto* plugin : loaded_plugins) {
        if (plugin->is_active && plugin->instance) {
            plugin->instance->stop();
            plugin->instance->cleanup();
        }
        loader_->unload_plugin(plugin->manifest.name);
    }

    environments_.clear();
}

bool PluginManager::scan_and_load_plugins() {
    bool success = true;

    for (const auto& plugin_dir : config_.plugin_directories) {
        if (!std::filesystem::exists(plugin_dir)) {
            std::cout << "Plugin directory not found: " << plugin_dir << std::endl;
            continue;
        }

        auto plugins = loader_->scan_plugin_directory(plugin_dir);
        for (const auto& plugin_path : plugins) {
            if (!load_plugin(plugin_path)) {
                std::cerr << "Failed to load plugin: " << plugin_path << std::endl;
                success = false;
            }
        }
    }

    return success;
}

bool PluginManager::load_plugin(const std::string& plugin_path) {
    if (!loader_->load_plugin(plugin_path)) {
        return false;
    }

    // Extract plugin name from path for environment creation
    std::filesystem::path path(plugin_path);
    std::string plugin_id = path.stem().string();

    // Create execution environment
    PluginContext context = create_plugin_context(plugin_id);
    environments_[plugin_id] = std::make_unique<PluginExecutionEnvironment>(context);

    emit_event(plugin_id, "loaded");
    return true;
}

PluginContext PluginManager::create_plugin_context(const std::string& plugin_id) const {
    PluginContext context;
    context.plugin_id = plugin_id;
    context.working_directory = std::filesystem::current_path().string();
    context.max_memory_mb = config_.default_security_policy.max_memory_mb;
    context.max_execution_time_ms = config_.default_security_policy.max_execution_time_ms;
    context.sandbox_enabled = config_.enable_sandbox;

    return context;
}

void PluginManager::emit_event(const std::string& plugin_id, const std::string& event,
                              const std::map<std::string, std::string>& data) {
    if (event_handler_) {
        event_handler_(plugin_id, event, data);
    }
}

std::vector<std::string> PluginManager::get_loaded_plugins() const {
    auto loaded_plugins = loader_->get_all_loaded_plugins();
    std::vector<std::string> plugin_ids;
    for (const auto* plugin : loaded_plugins) {
        plugin_ids.push_back(plugin->manifest.name);
    }
    return plugin_ids;
}

PluginManifest PluginManager::get_plugin_info(const std::string& plugin_id) const {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (plugin) {
        return plugin->manifest;
    }
    return {};
}

std::string PluginManager::get_plugin_status(const std::string& plugin_id) const {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin) {
        return "not_loaded";
    }

    if (plugin->is_active) {
        return plugin->instance ? plugin->instance->get_status() : "active";
    }

    return "loaded";
}

// Missing method implementations
bool PluginManager::unload_plugin(const std::string& plugin_id) {
    return loader_->unload_plugin(plugin_id);
}

bool PluginManager::enable_plugin(const std::string& plugin_id) {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin) {
        return false;
    }
    plugin->is_active = true;
    return true;
}

bool PluginManager::disable_plugin(const std::string& plugin_id) {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin) {
        return false;
    }
    plugin->is_active = false;
    return true;
}

std::vector<std::string> PluginManager::get_available_plugins() const {
    return get_loaded_plugins(); // Simplified implementation
}

std::vector<std::string> PluginManager::get_active_plugins() const {
    auto loaded_plugins = loader_->get_all_loaded_plugins();
    std::vector<std::string> active_plugins;
    for (const auto* plugin : loaded_plugins) {
        if (plugin->is_active) {
            active_plugins.push_back(plugin->manifest.name);
        }
    }
    return active_plugins;
}

PluginResult PluginManager::execute_plugin_command(const std::string& plugin_id,
                                                  const std::string& command,
                                                  const std::map<std::string, std::string>& parameters) {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin || !plugin->instance) {
        return {false, "Plugin not found or not loaded", {}, {}, {}, {}, 0};
    }

    return plugin->instance->execute_command(command, parameters);
}

bool PluginManager::configure_plugin(const std::string& plugin_id,
                                    const std::map<std::string, std::string>& config) {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin || !plugin->instance) {
        return false;
    }

    return plugin->instance->configure(config);
}

std::map<std::string, std::string> PluginManager::get_plugin_config(const std::string& plugin_id) const {
    auto* plugin = loader_->get_loaded_plugin(plugin_id);
    if (!plugin || !plugin->instance) {
        return {};
    }

    return plugin->instance->get_configuration_schema();
}

void PluginManager::stop_real_time_processing() {
    if (real_time_active_) {
        real_time_active_ = false;
        queue_cv_.notify_all();

        if (real_time_thread_.joinable()) {
            real_time_thread_.join();
        }
    }
}

// Missing PluginLoader method implementation
std::vector<std::string> PluginLoader::scan_plugin_directory(const std::string& directory) {
    std::vector<std::string> plugin_paths;

    if (!std::filesystem::exists(directory)) {
        return plugin_paths;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                auto extension = path.extension().string();

                // Look for shared libraries
#ifdef _WIN32
                if (extension == ".dll") {
#else
                if (extension == ".so") {
#endif
                    // Check if there's a corresponding plugin.json
                    auto manifest_path = path.parent_path() / "plugin.json";
                    if (std::filesystem::exists(manifest_path)) {
                        plugin_paths.push_back(path.string());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning plugin directory: " << e.what() << std::endl;
    }

    return plugin_paths;
}

// Missing PluginExecutionEnvironment implementations
PluginExecutionEnvironment::PluginExecutionEnvironment(const PluginContext& context)
    : context_(context) {
    if (context_.sandbox_enabled) {
        PluginSandbox::SecurityPolicy policy;
        policy.max_memory_mb = context_.max_memory_mb;
        policy.max_execution_time_ms = context_.max_execution_time_ms;
        sandbox_ = std::make_unique<PluginSandbox>(policy);
        sandbox_->initialize();
    }
}

PluginExecutionEnvironment::~PluginExecutionEnvironment() = default;

} // namespace netlogai::plugins