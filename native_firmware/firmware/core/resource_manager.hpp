#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace shaer {

struct ResourceHandle {
    std::string id;
    std::string path;
    bool cached = false;
};

class ResourceManager {
public:
    explicit ResourceManager(std::vector<std::string> roots = {});
    void add_root(std::string root);
    std::optional<ResourceHandle> load(const std::string& id, const std::string& relative_path);
    bool cached(const std::string& id) const;
    size_t cache_size() const;

private:
    std::vector<std::string> roots_;
    std::map<std::string, ResourceHandle> cache_;
};

}  // namespace shaer

