#include "music_library_store.hpp"

#include <cassert>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

std::string temp_root(const std::string& name) {
    return "/tmp/shaer_" + name + "_" + std::to_string(getpid());
}

void write_id3v1_mp3(const std::filesystem::path& path, const std::string& title, const std::string& artist, const std::string& album, unsigned char track) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << "FAKEAUDIO";
    char tag[128]{};
    std::memcpy(tag, "TAG", 3);
    std::memcpy(tag + 3, title.c_str(), std::min<size_t>(30, title.size()));
    std::memcpy(tag + 33, artist.c_str(), std::min<size_t>(30, artist.size()));
    std::memcpy(tag + 63, album.c_str(), std::min<size_t>(30, album.size()));
    tag[125] = 0;
    tag[126] = static_cast<char>(track);
    tag[127] = 17;
    file.write(tag, sizeof(tag));
}

void write_id3v2_album_art_mp3(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    const std::string title_payload = std::string(1, '\0') + "Cover Song";
    const std::string apic_payload = std::string(1, '\0') + "image/jpeg" + std::string(1, '\0') + std::string(1, '\3') + "cover" + std::string(1, '\0') + "JPEGDATA";
    const int tag_size = 10 + static_cast<int>(title_payload.size()) + 10 + static_cast<int>(apic_payload.size());
    auto sync = [](int value) {
        std::array<unsigned char, 4> out{};
        out[0] = static_cast<unsigned char>((value >> 21) & 0x7F);
        out[1] = static_cast<unsigned char>((value >> 14) & 0x7F);
        out[2] = static_cast<unsigned char>((value >> 7) & 0x7F);
        out[3] = static_cast<unsigned char>(value & 0x7F);
        return out;
    };
    auto be = [](int value) {
        std::array<unsigned char, 4> out{};
        out[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
        out[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
        out[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
        out[3] = static_cast<unsigned char>(value & 0xFF);
        return out;
    };

    std::ofstream file(path, std::ios::binary);
    file.write("ID3", 3);
    file.put(3);
    file.put(0);
    file.put(0);
    const auto size = sync(tag_size);
    file.write(reinterpret_cast<const char*>(size.data()), 4);
    file.write("TIT2", 4);
    const auto title_size = be(static_cast<int>(title_payload.size()));
    file.write(reinterpret_cast<const char*>(title_size.data()), 4);
    file.put(0);
    file.put(0);
    file.write(title_payload.data(), static_cast<std::streamsize>(title_payload.size()));
    file.write("APIC", 4);
    const auto apic_size = be(static_cast<int>(apic_payload.size()));
    file.write(reinterpret_cast<const char*>(apic_size.data()), 4);
    file.put(0);
    file.put(0);
    file.write(apic_payload.data(), static_cast<std::streamsize>(apic_payload.size()));
    file << "FAKEAUDIO";
}

void write_wav(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    const int sample_rate = 44100;
    const int bits = 16;
    const int channels = 2;
    const int byte_rate = sample_rate * channels * bits / 8;
    const int data_size = byte_rate;
    const int riff_size = 36 + data_size;
    auto put32 = [&file](int v) {
        file.put(static_cast<char>(v & 0xFF));
        file.put(static_cast<char>((v >> 8) & 0xFF));
        file.put(static_cast<char>((v >> 16) & 0xFF));
        file.put(static_cast<char>((v >> 24) & 0xFF));
    };
    auto put16 = [&file](int v) {
        file.put(static_cast<char>(v & 0xFF));
        file.put(static_cast<char>((v >> 8) & 0xFF));
    };
    file.write("RIFF", 4); put32(riff_size); file.write("WAVE", 4);
    file.write("fmt ", 4); put32(16); put16(1); put16(channels); put32(sample_rate); put32(byte_rate); put16(channels * bits / 8); put16(bits);
    file.write("data", 4); put32(data_size);
    std::vector<char> silence(static_cast<size_t>(data_size), 0);
    file.write(silence.data(), static_cast<std::streamsize>(silence.size()));
}

void scan_indexes_supported_files_and_categories() {
    const auto root = temp_root("library_scan");
    std::filesystem::remove_all(root);
    write_id3v1_mp3(std::filesystem::path(root) / "Album A" / "01-song.mp3", "Song One", "Artist A", "Album A", 1);
    write_wav(std::filesystem::path(root) / "Folder B" / "field.wav");
    std::ofstream(std::filesystem::path(root) / "Folder B" / "ignore.txt") << "no";

    shaer::MusicLibraryStore store(root + "/shaer.db");
    assert(store.open());
    assert(store.migrate());
    const auto scan = store.scan(root);
    assert(scan.files_seen == 2);
    assert(scan.tracks_indexed == 2);
    const auto songs = store.songs();
    assert(songs.size() == 2);
    const auto index = store.index();
    assert(!index.albums.empty());
    assert(!index.artists.empty());
    assert(!index.folders.empty());
    assert(store.search("Song").size() == 1);
    std::filesystem::remove_all(root);
}

void metadata_extraction_reads_id3_and_wav() {
    const auto root = temp_root("metadata");
    std::filesystem::remove_all(root);
    const auto mp3 = std::filesystem::path(root) / "song.mp3";
    const auto wav = std::filesystem::path(root) / "recording.wav";
    write_id3v1_mp3(mp3, "Tagged Title", "Tagged Artist", "Tagged Album", 7);
    write_wav(wav);
    shaer::MusicLibraryStore store(root + "/shaer.db");
    assert(store.open());
    assert(store.migrate());
    const auto mp3_meta = store.metadata_for_file(mp3.string()).value();
    assert(mp3_meta.title == "Tagged Title");
    assert(mp3_meta.artist == "Tagged Artist");
    assert(mp3_meta.album == "Tagged Album");
    assert(mp3_meta.track_number == 7);
    assert(mp3_meta.genre == "Rock");
    const auto wav_meta = store.metadata_for_file(wav.string()).value();
    assert(wav_meta.codec == "WAV");
    assert(wav_meta.sample_rate_hz == 44100);
    assert(wav_meta.bit_depth == 16);
    assert(wav_meta.duration_seconds == 1);
    std::filesystem::remove_all(root);
}

void album_art_is_cached_when_embedded() {
    const auto root = temp_root("album_art");
    std::filesystem::remove_all(root);
    write_id3v2_album_art_mp3(std::filesystem::path(root) / "cover.mp3");
    shaer::MusicLibraryStore store(root + "/data/shaer.db");
    assert(store.open());
    assert(store.migrate());
    const auto scan = store.scan(root);
    assert(scan.album_art_cached == 1);
    const auto songs = store.songs();
    assert(songs.size() == 1);
    assert(!songs.front().album_art_path.empty());
    assert(std::filesystem::exists(songs.front().album_art_path));
    std::filesystem::remove_all(root);
}

void incremental_scan_removes_deleted_files() {
    const auto root = temp_root("incremental");
    std::filesystem::remove_all(root);
    const auto file = std::filesystem::path(root) / "Album" / "gone.mp3";
    write_id3v1_mp3(file, "Gone", "Artist", "Album", 1);
    shaer::MusicLibraryStore store(root + "/shaer.db");
    assert(store.open());
    assert(store.migrate());
    assert(store.scan(root).tracks_indexed == 1);
    std::filesystem::remove(file);
    store.scan(root);
    assert(store.songs().empty());
    std::filesystem::remove_all(root);
}

void performance_scan_500_files_is_bounded() {
    const auto root = temp_root("performance");
    std::filesystem::remove_all(root);
    for (int i = 0; i < 500; ++i) {
        write_id3v1_mp3(
            std::filesystem::path(root) / ("Album " + std::to_string(i % 10)) / ("track_" + std::to_string(i) + ".mp3"),
            "Track " + std::to_string(i),
            "Artist " + std::to_string(i % 20),
            "Album " + std::to_string(i % 10),
            static_cast<unsigned char>((i % 12) + 1));
    }
    shaer::MusicLibraryStore store(root + "/shaer.db");
    assert(store.open());
    assert(store.migrate());
    const auto scan = store.scan(root);
    assert(scan.files_seen == 500);
    assert(scan.tracks_indexed == 500);
    assert(scan.elapsed_ms < 10000);
    std::filesystem::remove_all(root);
}

}  // namespace

int main() {
    scan_indexes_supported_files_and_categories();
    metadata_extraction_reads_id3_and_wav();
    album_art_is_cached_when_embedded();
    incremental_scan_removes_deleted_files();
    performance_scan_500_files_is_bounded();
    std::cout << "music_library_store_tests passed\n";
    return 0;
}
