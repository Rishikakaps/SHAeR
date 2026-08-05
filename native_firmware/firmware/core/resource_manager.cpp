#include "resource_manager.hpp"

#include <filesystem>
#include <utility>

namespace shaer {

ResourceManager::ResourceManager(std::vector<std::string> roots)
    : roots_(std::move(roots)) {}

void ResourceManager::add_root(std::string root) {
    roots_.push_back(std::move(root));
}

std::optional<ResourceHandle> ResourceManager::load(const std::string& id, const std::string& relative_path) {
    auto cached_item = cache_.find(id);
    if (cached_item != cache_.end()) {
        cached_item->second.cached = true;
        return cached_item->second;
    }
    for (const auto& root : roots_) {
        const std::filesystem::path candidate = std::filesystem::path(root) / relative_path;
        if (std::filesystem::exists(candidate)) {
            ResourceHandle handle{id, candidate.string(), false};
            cache_[id] = handle;
            return handle;
        }
    }
    return std::nullopt;
}

bool ResourceManager::cached(const std::string& id) const {
    return cache_.find(id) != cache_.end();
}

size_t ResourceManager::cache_size() const {
    return cache_.size();
}

}  // namespace shaer

