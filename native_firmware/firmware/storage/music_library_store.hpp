#pragma once

#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace shaer {

struct LibraryScanResult {
    int files_seen = 0;
    int tracks_indexed = 0;
    int tracks_added_or_updated = 0;
    int removed = 0;
    int album_art_cached = 0;
    int elapsed_ms = 0;
};

class MusicLibraryStore {
public:
    explicit MusicLibraryStore(std::string database_path);
    ~MusicLibraryStore();

    MusicLibraryStore(const MusicLibraryStore&) = delete;
    MusicLibraryStore& operator=(const MusicLibraryStore&) = delete;

    bool open();
    bool migrate();
    LibraryScanResult scan(const std::string& music_directory);
    std::vector<Track> songs(int limit = 0) const;
    LibraryIndex index() const;
    std::vector<Track> search(const std::string& query, int limit = 50) const;
    std::optional<Track> metadata_for_file(const std::string& path) const;
    std::string last_error() const;
    std::string path() const;

private:
    Track extract_metadata(const std::string& file_path, const std::string& music_root, int* album_art_cached) const;
    bool upsert_track(const Track& track, long long size_bytes, long long modified_at, bool* changed);
    bool remove_missing();
    bool exec(const std::string& sql) const;
    bool prepare_parent_directory() const;
    std::vector<std::string> distinct_text(const std::string& column) const;

    std::string database_path_;
    sqlite3* db_ = nullptr;
    mutable std::string last_error_;
};

bool is_supported_audio_file(const std::string& path);

}  // namespace shaer
