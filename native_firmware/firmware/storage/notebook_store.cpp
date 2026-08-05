#include "notebook_store.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

namespace shaer {

namespace {

long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string value_after_equals(const std::string& line) {
    const auto position = line.find('=');
    return position == std::string::npos ? std::string{} : line.substr(position + 1);
}

}  // namespace

NotebookStore::NotebookStore(std::filesystem::path root) : root_(std::move(root)) {}

std::string NotebookStore::safe_component(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') || character == '_' || character == '-') {
            result.push_back(character);
        } else {
            result.push_back('_');
        }
    }
    return result.empty() ? "unknown" : result;
}

std::filesystem::path NotebookStore::notebook_directory(const Notebook& notebook) const {
    return root_ / safe_component(notebook.song_id);
}

Notebook NotebookStore::load_or_create(const Track& song) {
    Notebook notebook;
    notebook.id = "notebook:" + song.id;
    notebook.song_id = song.id;
    notebook.source = song.source;
    notebook.title = song.title;
    notebook.artist = song.artist;
    notebook.album = song.album;

    const auto directory = notebook_directory(notebook);
    std::ifstream metadata(directory / "metadata.txt");
    std::string line;
    while (std::getline(metadata, line)) {
        if (line.rfind("id=", 0) == 0) notebook.id = value_after_equals(line);
        else if (line.rfind("created_at=", 0) == 0) notebook.created_at = std::stoll(value_after_equals(line));
        else if (line.rfind("modified_at=", 0) == 0) notebook.modified_at = std::stoll(value_after_equals(line));
        else if (line.rfind("thumbnail=", 0) == 0) notebook.thumbnail_path = value_after_equals(line);
    }
    for (int page_number = 1; page_number <= 999; ++page_number) {
        const auto page_path = directory / (std::string("page") + (page_number < 10 ? "00" : page_number < 100 ? "0" : "") + std::to_string(page_number) + ".strokes");
        std::ifstream page_file(page_path);
        if (!page_file) break;
        NotebookPage page;
        page.id = notebook.id + ":page" + std::to_string(page_number);
        std::string stroke_line;
        while (std::getline(page_file, stroke_line)) {
            std::stringstream stream(stroke_line);
            NotebookPage::Stroke stroke;
            char separator = 0;
            if (stream >> stroke.start_x >> separator >> stroke.start_y >> separator
                >> stroke.end_x >> separator >> stroke.end_y >> separator
                >> stroke.width >> separator >> stroke.colour >> separator
                >> stroke.pressure >> separator >> stroke.timestamp) {
                page.strokes.push_back(stroke);
            }
        }
        page.revision = static_cast<int>(page.strokes.size());
        page.created_at = notebook.created_at;
        page.modified_at = notebook.modified_at;
        notebook.pages.push_back(std::move(page));
    }
    notebook.page_count = static_cast<int>(notebook.pages.size());
    if (notebook.created_at == 0) notebook.created_at = now_ms();
    if (notebook.modified_at == 0) notebook.modified_at = notebook.created_at;
    if (notebook.pages.empty()) {
        NotebookPage page;
        page.id = notebook.id + ":page001";
        page.created_at = notebook.created_at;
        page.modified_at = notebook.modified_at;
        notebook.pages.push_back(std::move(page));
        notebook.page_count = 1;
        save(notebook);
    }
    return notebook;
}

bool NotebookStore::save(const Notebook& notebook) const {
    if (notebook.song_id.empty()) return false;
    const auto directory = notebook_directory(notebook);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return false;

    const auto metadata_tmp = directory / "metadata.txt.tmp";
    {
        std::ofstream metadata(metadata_tmp, std::ios::trunc);
        if (!metadata) return false;
        metadata << "id=" << notebook.id << '\n'
                 << "song_id=" << notebook.song_id << '\n'
                 << "created_at=" << notebook.created_at << '\n'
                 << "modified_at=" << notebook.modified_at << '\n'
                 << "thumbnail=" << notebook.thumbnail_path << '\n';
    }
    std::filesystem::rename(metadata_tmp, directory / "metadata.txt", error);
    if (error) return false;

    for (size_t index = 0; index < notebook.pages.size(); ++index) {
        const auto page_number = static_cast<int>(index + 1);
        const auto name = std::string("page") + (page_number < 10 ? "00" : page_number < 100 ? "0" : "") + std::to_string(page_number) + ".strokes";
        const auto temporary = directory / (name + ".tmp");
        std::ofstream page_file(temporary, std::ios::trunc);
        if (!page_file) return false;
        for (const auto& stroke : notebook.pages[index].strokes) {
            page_file << stroke.start_x << ',' << stroke.start_y << ','
                      << stroke.end_x << ',' << stroke.end_y << ','
                      << stroke.width << ',' << stroke.colour << ','
                      << stroke.pressure << ',' << stroke.timestamp << '\n';
        }
        page_file.close();
        std::filesystem::rename(temporary, directory / name, error);
        if (error) return false;
    }
    return true;
}

bool NotebookStore::remove(const Notebook& notebook) const {
    std::error_code error;
    std::filesystem::remove_all(notebook_directory(notebook), error);
    return !error;
}

}  // namespace shaer
