#include "music_library_store.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sqlite3.h>
#include <sstream>
#include <utility>

namespace shaer {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim(std::string value) {
    while (!value.empty() && (value.back() == '\0' || std::isspace(static_cast<unsigned char>(value.back())))) {
        value.pop_back();
    }
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    return value.substr(start);
}

std::string fixed_field(const char* data, size_t len) {
    return trim(std::string(data, data + len));
}

int syncsafe(const unsigned char* bytes) {
    return ((bytes[0] & 0x7F) << 21) | ((bytes[1] & 0x7F) << 14) | ((bytes[2] & 0x7F) << 7) | (bytes[3] & 0x7F);
}

int be32(const unsigned char* bytes) {
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

int le16(const unsigned char* bytes) {
    return bytes[0] | (bytes[1] << 8);
}

int le32(const unsigned char* bytes) {
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

std::string text_frame_value(const std::vector<unsigned char>& data, size_t offset, size_t size) {
    if (size <= 1 || offset + size > data.size()) return {};
    const unsigned char encoding = data[offset];
    std::string raw(reinterpret_cast<const char*>(&data[offset + 1]), size - 1);
    if (encoding == 1 || encoding == 2) {
        std::string ascii;
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            const unsigned char a = static_cast<unsigned char>(raw[i]);
            const unsigned char b = static_cast<unsigned char>(raw[i + 1]);
            const unsigned char c = a == 0 ? b : a;
            if (c != 0) ascii.push_back(static_cast<char>(c));
        }
        return trim(ascii);
    }
    return trim(raw);
}

std::string genre_from_id3v1(unsigned char genre) {
    static const char* genres[] = {
        "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
        "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae",
        "Rock", "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
        "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    };
    if (genre < sizeof(genres) / sizeof(genres[0])) return genres[genre];
    return {};
}

std::string codec_for_extension(const std::filesystem::path& path) {
    const std::string ext = lower(path.extension().string());
    if (ext == ".mp3") return "MP3";
    if (ext == ".flac") return "FLAC";
    if (ext == ".wav") return "WAV";
    if (ext == ".ogg") return "OGG";
    return {};
}

std::string cache_art_path(const std::string& db_path, const std::string& file_path) {
    const auto base = std::filesystem::path(db_path).parent_path() / "cover_art_cache";
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    const auto hash = std::hash<std::string>{}(file_path);
    return (base / (std::to_string(hash) + ".art")).string();
}

void parse_id3v1(const std::string& path, Track* track) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    if (size < 128) return;
    file.seekg(-128, std::ios::end);
    char tag[128]{};
    file.read(tag, sizeof(tag));
    if (std::strncmp(tag, "TAG", 3) != 0) return;
    const auto title = fixed_field(tag + 3, 30);
    const auto artist = fixed_field(tag + 33, 30);
    const auto album = fixed_field(tag + 63, 30);
    if (!title.empty()) track->title = title;
    if (!artist.empty()) track->artist = artist;
    if (!album.empty()) track->album = album;
    if (static_cast<unsigned char>(tag[125]) == 0 && static_cast<unsigned char>(tag[126]) != 0) {
        track->track_number = static_cast<unsigned char>(tag[126]);
    }
    track->genre = genre_from_id3v1(static_cast<unsigned char>(tag[127]));
}

void parse_wav(const std::string& path, Track* track) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;
    std::vector<unsigned char> header(64 * 1024);
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    const size_t read = static_cast<size_t>(file.gcount());
    if (read < 44 || std::memcmp(header.data(), "RIFF", 4) != 0 || std::memcmp(header.data() + 8, "WAVE", 4) != 0) return;
    size_t pos = 12;
    int byte_rate = 0;
    int data_bytes = 0;
    while (pos + 8 <= read) {
        const std::string chunk(reinterpret_cast<const char*>(&header[pos]), 4);
        const int chunk_size = le32(&header[pos + 4]);
        pos += 8;
        if (chunk == "fmt " && pos + 16 <= read) {
            track->sample_rate_hz = le32(&header[pos + 4]);
            byte_rate = le32(&header[pos + 8]);
            track->bit_depth = le16(&header[pos + 14]);
        } else if (chunk == "data") {
            data_bytes = chunk_size;
        }
        pos += static_cast<size_t>(std::max(0, chunk_size));
    }
    if (byte_rate > 0 && data_bytes > 0) {
        track->duration_seconds = data_bytes / byte_rate;
        track->bitrate_kbps = byte_rate * 8 / 1000;
    }
}

void parse_id3v2(const std::string& path, const std::string& db_path, Track* track, int* album_art_cached) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;
    std::vector<unsigned char> data(512 * 1024);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    data.resize(static_cast<size_t>(file.gcount()));
    if (data.size() < 10 || std::memcmp(data.data(), "ID3", 3) != 0) return;
    const int version = data[3];
    const int tag_size = syncsafe(&data[6]);
    size_t pos = 10;
    const size_t end = std::min(data.size(), static_cast<size_t>(10 + std::max(0, tag_size)));
    while (pos + 10 <= end) {
        const std::string id(reinterpret_cast<const char*>(&data[pos]), 4);
        if (id[0] == '\0') break;
        const int frame_size = version == 4 ? syncsafe(&data[pos + 4]) : be32(&data[pos + 4]);
        pos += 10;
        if (frame_size <= 0 || pos + static_cast<size_t>(frame_size) > end) break;
        if (id == "TIT2") {
            const auto value = text_frame_value(data, pos, static_cast<size_t>(frame_size));
            if (!value.empty()) track->title = value;
        } else if (id == "TPE1") {
            const auto value = text_frame_value(data, pos, static_cast<size_t>(frame_size));
            if (!value.empty()) track->artist = value;
        } else if (id == "TALB") {
            const auto value = text_frame_value(data, pos, static_cast<size_t>(frame_size));
            if (!value.empty()) track->album = value;
        } else if (id == "TRCK") {
            const auto value = text_frame_value(data, pos, static_cast<size_t>(frame_size));
            try { track->track_number = std::stoi(value); } catch (...) {}
        } else if (id == "TCON") {
            const auto value = text_frame_value(data, pos, static_cast<size_t>(frame_size));
            if (!value.empty()) track->genre = value;
        } else if (id == "APIC" && frame_size > 16) {
            size_t art = pos + 1;
            while (art < pos + static_cast<size_t>(frame_size) && data[art] != 0) ++art;
            if (art + 3 < pos + static_cast<size_t>(frame_size)) {
                art += 2;
                while (art < pos + static_cast<size_t>(frame_size) && data[art] != 0) ++art;
                ++art;
                if (art < pos + static_cast<size_t>(frame_size)) {
                    const std::string out = cache_art_path(db_path, path);
                    std::ofstream cover(out, std::ios::binary);
                    cover.write(reinterpret_cast<const char*>(&data[art]), static_cast<std::streamsize>(pos + static_cast<size_t>(frame_size) - art));
                    if (cover) {
                        track->album_art_path = out;
                        if (album_art_cached) ++(*album_art_cached);
                    }
                }
            }
        }
        pos += static_cast<size_t>(frame_size);
    }
}

}  // namespace

bool is_supported_audio_file(const std::string& path) {
    const std::string ext = lower(std::filesystem::path(path).extension().string());
    return ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".ogg";
}

MusicLibraryStore::MusicLibraryStore(std::string database_path)
    : database_path_(std::move(database_path)) {}

MusicLibraryStore::~MusicLibraryStore() {
    if (db_) sqlite3_close(db_);
}

bool MusicLibraryStore::open() {
    if (!prepare_parent_directory()) return false;
    if (sqlite3_open(database_path_.c_str(), &db_) != SQLITE_OK) {
        last_error_ = db_ ? sqlite3_errmsg(db_) : "sqlite open failed";
        return false;
    }
    sqlite3_busy_timeout(db_, 2500);
    return exec("PRAGMA journal_mode=WAL;") && exec("PRAGMA foreign_keys=ON;");
}

bool MusicLibraryStore::migrate() {
    return exec(
        "CREATE TABLE IF NOT EXISTS library_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS tracks("
        "id INTEGER PRIMARY KEY,"
        "path TEXT NOT NULL UNIQUE,"
        "title TEXT NOT NULL,"
        "artist TEXT NOT NULL,"
        "album TEXT NOT NULL,"
        "track_number INTEGER NOT NULL DEFAULT 0,"
        "genre TEXT NOT NULL DEFAULT '',"
        "duration_seconds INTEGER NOT NULL DEFAULT 0,"
        "codec TEXT NOT NULL,"
        "bitrate_kbps INTEGER NOT NULL DEFAULT 0,"
        "sample_rate_hz INTEGER NOT NULL DEFAULT 0,"
        "bit_depth INTEGER NOT NULL DEFAULT 0,"
        "album_art_path TEXT NOT NULL DEFAULT '',"
        "folder TEXT NOT NULL DEFAULT '',"
        "size_bytes INTEGER NOT NULL DEFAULT 0,"
        "modified_at INTEGER NOT NULL DEFAULT 0,"
        "added_at INTEGER NOT NULL DEFAULT (unixepoch()),"
        "last_played INTEGER,"
        "play_count INTEGER NOT NULL DEFAULT 0,"
        "favorite INTEGER NOT NULL DEFAULT 0,"
        "scan_seen INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_tracks_title ON tracks(title);"
        "CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album);"
        "CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist);"
        "CREATE INDEX IF NOT EXISTS idx_tracks_folder ON tracks(folder);"
        "CREATE VIRTUAL TABLE IF NOT EXISTS track_search USING fts5(title, artist, album, folder, path UNINDEXED);"
        "INSERT OR REPLACE INTO library_meta(key, value) VALUES('schema_version', '1');");
}

LibraryScanResult MusicLibraryStore::scan(const std::string& music_directory) {
    const auto started = std::chrono::steady_clock::now();
    LibraryScanResult result;
    if (!db_ && !open()) return result;
    if (!migrate()) return result;
    exec("UPDATE tracks SET scan_seen = 0;");

    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(music_directory, ec)) {
        fs::create_directories(music_directory, ec);
    }
    for (const auto& entry : fs::recursive_directory_iterator(music_directory, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!entry.is_regular_file(ec)) continue;
        const std::string path = entry.path().string();
        if (!is_supported_audio_file(path)) continue;
        ++result.files_seen;
        int art = 0;
        Track track = extract_metadata(path, music_directory, &art);
        result.album_art_cached += art;
        const auto modified = entry.last_write_time(ec).time_since_epoch().count();
        const auto size = static_cast<long long>(entry.file_size(ec));
        bool changed = false;
        if (upsert_track(track, size, static_cast<long long>(modified), &changed)) {
            ++result.tracks_indexed;
            if (changed) ++result.tracks_added_or_updated;
        }
    }
    if (remove_missing()) {
        result.removed = sqlite3_changes(db_);
    }
    result.elapsed_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count());
    return result;
}

Track MusicLibraryStore::extract_metadata(const std::string& file_path, const std::string& music_root, int* album_art_cached) const {
    const std::filesystem::path path(file_path);
    Track track;
    track.title = path.stem().string();
    track.artist = "Unknown Artist";
    track.album = path.parent_path().filename().string().empty() ? "Unknown Album" : path.parent_path().filename().string();
    track.file_path = file_path;
    track.source = PlaybackSource::Local;
    track.codec = codec_for_extension(path);
    std::error_code ec;
    const auto relative = std::filesystem::relative(path.parent_path(), music_root, ec);
    track.folder = ec ? path.parent_path().string() : relative.string();
    if (track.folder == ".") track.folder = "/";
    if (track.codec == "MP3") {
        parse_id3v1(file_path, &track);
        parse_id3v2(file_path, database_path_, &track, album_art_cached);
    } else if (track.codec == "WAV") {
        parse_wav(file_path, &track);
    }
    return track;
}

bool MusicLibraryStore::upsert_track(const Track& track, long long size_bytes, long long modified_at, bool* changed) {
    const char* probe_sql = "SELECT size_bytes, modified_at FROM tracks WHERE path = ?;";
    sqlite3_stmt* probe = nullptr;
    bool needs_write = true;
    if (sqlite3_prepare_v2(db_, probe_sql, -1, &probe, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(probe, 1, track.file_path.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(probe) == SQLITE_ROW) {
            needs_write = sqlite3_column_int64(probe, 0) != size_bytes || sqlite3_column_int64(probe, 1) != modified_at;
        }
    }
    sqlite3_finalize(probe);
    if (changed) *changed = needs_write;

    const char* sql =
        "INSERT INTO tracks(path,title,artist,album,track_number,genre,duration_seconds,codec,bitrate_kbps,sample_rate_hz,bit_depth,album_art_path,folder,size_bytes,modified_at,scan_seen)"
        " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,1)"
        " ON CONFLICT(path) DO UPDATE SET "
        "title=excluded.title,artist=excluded.artist,album=excluded.album,track_number=excluded.track_number,genre=excluded.genre,"
        "duration_seconds=excluded.duration_seconds,codec=excluded.codec,bitrate_kbps=excluded.bitrate_kbps,sample_rate_hz=excluded.sample_rate_hz,"
        "bit_depth=excluded.bit_depth,album_art_path=excluded.album_art_path,folder=excluded.folder,size_bytes=excluded.size_bytes,modified_at=excluded.modified_at,scan_seen=1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, track.file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, track.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, track.artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, track.album.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, track.track_number);
    sqlite3_bind_text(stmt, 6, track.genre.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, track.duration_seconds);
    sqlite3_bind_text(stmt, 8, track.codec.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, track.bitrate_kbps);
    sqlite3_bind_int(stmt, 10, track.sample_rate_hz);
    sqlite3_bind_int(stmt, 11, track.bit_depth);
    sqlite3_bind_text(stmt, 12, track.album_art_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, track.folder.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 14, size_bytes);
    sqlite3_bind_int64(stmt, 15, modified_at);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    const char* fts_delete = "DELETE FROM track_search WHERE rowid = (SELECT id FROM tracks WHERE path = ?);";
    sqlite3_stmt* fts_delete_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, fts_delete, -1, &fts_delete_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(fts_delete_stmt, 1, track.file_path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(fts_delete_stmt);
    }
    sqlite3_finalize(fts_delete_stmt);

    const char* fts =
        "INSERT INTO track_search(rowid,title,artist,album,folder,path) "
        "VALUES((SELECT id FROM tracks WHERE path=?),?,?,?,?,?);";
    sqlite3_stmt* fts_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, fts, -1, &fts_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(fts_stmt, 1, track.file_path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(fts_stmt, 2, track.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(fts_stmt, 3, track.artist.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(fts_stmt, 4, track.album.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(fts_stmt, 5, track.folder.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(fts_stmt, 6, track.file_path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(fts_stmt);
    }
    sqlite3_finalize(fts_stmt);
    return true;
}

bool MusicLibraryStore::remove_missing() {
    return exec(
        "DELETE FROM track_search WHERE rowid IN (SELECT id FROM tracks WHERE scan_seen = 0);"
        "DELETE FROM tracks WHERE scan_seen = 0;");
}

std::vector<Track> MusicLibraryStore::songs(int limit) const {
    std::vector<Track> out;
    std::string sql =
        "SELECT title,artist,album,path,track_number,genre,duration_seconds,codec,bitrate_kbps,sample_rate_hz,bit_depth,album_art_path,folder "
        "FROM tracks ORDER BY artist, album, track_number, title";
    if (limit > 0) sql += " LIMIT " + std::to_string(limit);
    sql += ";";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Track t;
        t.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t.artist = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.album = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.source = PlaybackSource::Local;
        t.track_number = sqlite3_column_int(stmt, 4);
        t.genre = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        t.duration_seconds = sqlite3_column_int(stmt, 6);
        t.codec = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        t.bitrate_kbps = sqlite3_column_int(stmt, 8);
        t.sample_rate_hz = sqlite3_column_int(stmt, 9);
        t.bit_depth = sqlite3_column_int(stmt, 10);
        t.album_art_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        t.folder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        out.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return out;
}

LibraryIndex MusicLibraryStore::index() const {
    LibraryIndex index;
    index.albums = distinct_text("album");
    index.artists = distinct_text("artist");
    index.folders = distinct_text("folder");
    for (const auto& track : songs()) {
        index.songs.push_back(track.title);
    }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT title FROM tracks ORDER BY added_at DESC, title LIMIT 50;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            index.recently_added.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
    }
    sqlite3_finalize(stmt);
    return index;
}

std::vector<Track> MusicLibraryStore::search(const std::string& query, int limit) const {
    std::vector<Track> out;
    if (query.empty()) return out;
    const char* sql =
        "SELECT t.title,t.artist,t.album,t.path,t.track_number,t.genre,t.duration_seconds,t.codec,t.bitrate_kbps,t.sample_rate_hz,t.bit_depth,t.album_art_path,t.folder "
        "FROM track_search s JOIN tracks t ON t.id = s.rowid WHERE track_search MATCH ? ORDER BY rank LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return out;
    const std::string match = query + "*";
    sqlite3_bind_text(stmt, 1, match.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, std::max(1, limit));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Track t;
        t.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t.artist = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.album = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.source = PlaybackSource::Local;
        t.track_number = sqlite3_column_int(stmt, 4);
        t.genre = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        t.duration_seconds = sqlite3_column_int(stmt, 6);
        t.codec = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        t.bitrate_kbps = sqlite3_column_int(stmt, 8);
        t.sample_rate_hz = sqlite3_column_int(stmt, 9);
        t.bit_depth = sqlite3_column_int(stmt, 10);
        t.album_art_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        t.folder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        out.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::optional<Track> MusicLibraryStore::metadata_for_file(const std::string& path) const {
    if (!is_supported_audio_file(path)) return std::nullopt;
    int art = 0;
    return extract_metadata(path, std::filesystem::path(path).parent_path().string(), &art);
}

std::string MusicLibraryStore::last_error() const {
    return last_error_;
}

std::string MusicLibraryStore::path() const {
    return database_path_;
}

bool MusicLibraryStore::exec(const std::string& sql) const {
    char* error = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        last_error_ = error ? error : sqlite3_errmsg(db_);
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool MusicLibraryStore::prepare_parent_directory() const {
    const std::filesystem::path path(database_path_);
    const auto parent = path.parent_path();
    if (parent.empty()) return true;
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        last_error_ = ec.message();
        return false;
    }
    return true;
}

std::vector<std::string> MusicLibraryStore::distinct_text(const std::string& column) const {
    std::vector<std::string> out;
    const std::string sql = "SELECT DISTINCT " + column + " FROM tracks WHERE " + column + " != '' ORDER BY " + column + ";";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        out.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return out;
}

}  // namespace shaer
