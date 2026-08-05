#pragma once

#include "types.hpp"

#include <filesystem>
#include <string>

namespace shaer {

// Notebook persistence is deliberately independent from the music directory.
// The file format is small, deterministic, and can be replaced by sync later.
class NotebookStore {
public:
    explicit NotebookStore(std::filesystem::path root = "data/notes");

    Notebook load_or_create(const Track& song);
    bool save(const Notebook& notebook) const;
    bool remove(const Notebook& notebook) const;
    const std::filesystem::path& root() const { return root_; }

private:
    std::filesystem::path notebook_directory(const Notebook& notebook) const;
    static std::string safe_component(const std::string& value);
    std::filesystem::path root_;
};

}  // namespace shaer
