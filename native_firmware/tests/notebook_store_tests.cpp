#include "notebook_store.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    const auto root = std::filesystem::temp_directory_path() / "shaer-notebook-store-test";
    std::filesystem::remove_all(root);

    shaer::Track song;
    song.id = "local:/music/remember.flac";
    song.title = "Remember";
    song.artist = "SHAeR Archive";
    song.album = "Notes";
    song.source = shaer::PlaybackSource::Local;

    shaer::NotebookStore store(root);
    auto notebook = store.load_or_create(song);
    notebook.pages.push_back({});
    notebook.pages.back().id = notebook.id + ":page002";
    notebook.pages.back().strokes.push_back({10, 12, 20, 24, 2, 7, 0, 1234});
    notebook.page_count = static_cast<int>(notebook.pages.size());
    assert(store.save(notebook));

    const auto restored = store.load_or_create(song);
    assert(restored.id == notebook.id);
    assert(restored.song_id == song.id);
    assert(restored.page_count == 2);
    assert(restored.pages.back().strokes.size() == 1);
    assert(restored.pages.back().strokes.front().end_x == 20);
    assert(store.remove(restored));
    assert(!std::filesystem::exists(root / "local__music_remember_flac"));
    std::error_code cleanup_error;
    std::filesystem::remove(root, cleanup_error);

    std::cout << "notebook_store_tests passed\n";
    return 0;
}
