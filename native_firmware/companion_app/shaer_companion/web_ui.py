from __future__ import annotations

import json
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from .app_core import CompanionAppCore


HTML = """<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SHAeR Companion</title>
  <style>
    :root {
      color-scheme: dark;
      --bg:#0d1017; --panel:#151a24; --panel2:#101520; --line:#2b3344;
      --text:#f5f1e7; --muted:#aab0c1; --accent:#b09ad6; --hot:#b55c30;
      --good:#77d88d; --warn:#e2b66d;
    }
    * { box-sizing: border-box; }
    body { margin:0; background:var(--bg); color:var(--text); font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; }
    header { display:flex; align-items:center; justify-content:space-between; padding:16px 22px; border-bottom:1px solid var(--line); background:#090c12; }
    h1 { margin:0; font-size:22px; letter-spacing:0; }
    .app { display:grid; grid-template-columns:220px 1fr; min-height:calc(100vh - 58px); }
    nav { border-right:1px solid var(--line); background:#101520; padding:12px; }
    nav button { display:block; width:100%; margin:0 0 8px; text-align:left; }
    main { padding:18px; overflow:auto; }
    button { border:0; border-radius:6px; padding:9px 11px; background:#2b3344; color:var(--text); font-weight:700; cursor:pointer; }
    button.primary { background:var(--accent); color:#111; }
    button.hot { background:var(--hot); color:#fff7ef; }
    nav button.active { background:var(--accent); color:#111; }
    input, select, textarea { width:100%; border:1px solid var(--line); background:#0b0f17; color:var(--text); border-radius:6px; padding:9px; }
    label { display:block; font-size:12px; color:var(--muted); margin:0 0 6px; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(220px,1fr)); gap:12px; }
    .card { background:var(--panel); border:1px solid var(--line); border-radius:8px; padding:13px; margin-bottom:12px; }
    .stat { font-size:26px; font-weight:800; }
    .muted { color:var(--muted); }
    .row { display:grid; grid-template-columns:1fr auto; gap:10px; align-items:center; border-top:1px solid var(--line); padding:9px 0; }
    .row:first-child { border-top:0; }
    table { width:100%; border-collapse:collapse; border:1px solid var(--line); background:var(--panel); border-radius:8px; overflow:hidden; }
    th, td { padding:8px 9px; border-bottom:1px solid var(--line); text-align:left; font-size:13px; vertical-align:top; }
    th { color:var(--muted); background:var(--panel2); }
    tr:hover td { background:#1a2130; }
    .two { display:grid; grid-template-columns:1fr 1fr; gap:12px; }
    .status { color:var(--muted); white-space:pre-wrap; }
    .pill { display:inline-block; padding:3px 7px; border-radius:999px; background:#232b3b; color:var(--accent); font-size:12px; }
    .section { display:none; }
    .section.active { display:block; }
    pre { white-space:pre-wrap; overflow:auto; max-height:320px; background:#0b0f17; border:1px solid var(--line); border-radius:8px; padding:10px; }
    @media (max-width:760px) { .app { grid-template-columns:1fr; } nav { display:grid; grid-template-columns:repeat(2,1fr); gap:8px; } nav button { margin:0; } .two { grid-template-columns:1fr; } }
  </style>
</head>
<body>
  <header><h1>SHAeR Companion</h1><span class="pill">Desktop Manager</span></header>
  <div class="app">
    <nav>
      <button class="active" data-tab="dashboard">Dashboard</button>
      <button data-tab="library">Library</button>
      <button data-tab="playlists">Playlists</button>
      <button data-tab="duplicates">Duplicates</button>
      <button data-tab="themes">Themes</button>
      <button data-tab="settings">Settings</button>
      <button data-tab="firmware">Firmware</button>
      <button data-tab="voice">Voice Notes</button>
      <button data-tab="sync">Sync</button>
      <button data-tab="logs">Logs</button>
    </nav>
    <main>
      <section id="dashboard" class="section active">
        <div class="grid" id="stats"></div>
        <div class="card"><h2>Quick Import</h2><label>Music folder</label><input id="importPath" placeholder="/Users/rishika/Music"><button class="primary" onclick="importMusic()">Import Music</button></div>
      </section>
      <section id="library" class="section"><div class="card"><h2>Library Manager</h2><table><thead><tr><th>Title</th><th>Artist</th><th>Album</th><th>Format</th><th>Year</th><th>Metadata</th></tr></thead><tbody id="tracks"></tbody></table></div><div class="card"><h2>Metadata Viewer</h2><pre id="metadata">Select a track.</pre></div></section>
      <section id="playlists" class="section"><div class="card"><h2>Playlist Editor</h2><div class="two"><div><label>Playlist name</label><input id="playlistName" placeholder="Road Trip"></div><div><label>Track hashes, one per line</label><textarea id="playlistTracks" rows="4"></textarea></div></div><button class="primary" onclick="savePlaylist()">Save Playlist</button></div><div class="card" id="playlistList"></div></section>
      <section id="duplicates" class="section"><div class="card"><h2>Duplicate Finder</h2><button onclick="refreshDuplicates()">Scan Duplicates</button><table><thead><tr><th>Track</th><th>Copies</th><th>Paths</th></tr></thead><tbody id="duplicatesBody"></tbody></table></div></section>
      <section id="themes" class="section"><div class="card"><h2>Theme Installer</h2><label>Theme folder path</label><input id="themePath" placeholder="/path/to/theme_folder"><button class="primary" onclick="installTheme()">Install Theme</button></div><div class="card"><h2>Installed Themes</h2><div id="themeList"></div></div></section>
      <section id="settings" class="section"><div class="card"><h2>Device Settings</h2><div class="grid"><div><label>Wi-Fi SSID</label><input id="wifiSsid"></div><div><label>Bluetooth headphones</label><input id="btName"></div><div><label>Crossfade seconds</label><input id="crossfade" type="number" min="0" max="12"></div><div><label>ReplayGain</label><select id="replaygain"><option>off</option><option>track</option><option>album</option></select></div><div><label>Volume limit</label><input id="volumeLimit" type="number" min="1" max="100"></div><div><label>Recording quality</label><select id="recordingQuality"><option>mp3</option><option>wav</option></select></div></div><button class="primary" onclick="saveSettings()">Save Settings</button></div><div class="card"><h2>Stored Settings</h2><pre id="settingsView"></pre></div></section>
      <section id="firmware" class="section"><div class="card"><h2>Firmware Updater</h2><div class="two"><div><label>Version</label><input id="firmwareVersion" placeholder="1.0.1"></div><div><label>Firmware package path</label><input id="firmwarePath" placeholder="/path/to/shaer.bin"></div></div><button onclick="registerFirmware()">Register</button><label>Device/SD path</label><input id="firmwareDevice" placeholder="/Volumes/SHAER_SD"><button class="primary" onclick="stageFirmware()">Stage Update</button></div><div class="card"><h2>Registered Firmware</h2><pre id="firmwareList"></pre></div></section>
      <section id="voice" class="section"><div class="card"><h2>Voice Notes</h2><div class="grid"><div><label>Title</label><input id="voiceTitle"></div><div><label>Audio file path</label><input id="voicePath"></div><div><label>Linked type</label><select id="voiceType"><option>track</option><option>album</option><option>playlist</option></select></div><div><label>Linked ID/hash/name</label><input id="voiceLinked"></div></div><button class="primary" onclick="addVoice()">Add Voice Note</button></div><div class="card"><h2>Memory Mode Index</h2><table><thead><tr><th>Title</th><th>Link</th><th>Format</th><th>Path</th></tr></thead><tbody id="voiceList"></tbody></table></div></section>
      <section id="sync" class="section"><div class="card"><h2>SD Card Synchronization</h2><label>SHAeR SD card root</label><input id="devicePath" placeholder="/Volumes/SHAER_SD"><button onclick="previewSync()">Preview</button><button class="primary" onclick="syncDevice()">Sync</button></div><pre id="syncPreview">No sync preview yet.</pre></section>
      <section id="logs" class="section"><div class="card"><h2>Diagnostics + Logs</h2><button onclick="refreshAll()">Refresh Everything</button><pre id="status">Ready</pre></div></section>
    </main>
  </div>
  <script>
    const tabs = document.querySelectorAll("nav button");
    tabs.forEach(button => button.addEventListener("click", () => {
      tabs.forEach(b => b.classList.remove("active"));
      document.querySelectorAll(".section").forEach(s => s.classList.remove("active"));
      button.classList.add("active");
      document.getElementById(button.dataset.tab).classList.add("active");
    }));
    async function api(path, options) {
      const response = await fetch(path, options || {});
      const data = await response.json();
      if (!response.ok) throw new Error(data.error || "Request failed");
      return data;
    }
    function esc(value) { return String(value ?? "").replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c])); }
    function status(text) { document.getElementById("status").textContent = text; }
    async function refreshStats() {
      const data = await api("/api/stats");
      document.getElementById("stats").innerHTML = Object.entries(data).map(([k,v]) => `<div class="card"><div class="stat">${esc(v)}</div><div class="muted">${esc(k)}</div></div>`).join("");
    }
    async function refreshTracks() {
      const data = await api("/api/tracks");
      document.getElementById("tracks").innerHTML = data.tracks.map(t => `<tr><td>${esc(t.title)}</td><td>${esc(t.artist)}</td><td>${esc(t.album)}</td><td>${esc(t.file_format)}</td><td>${esc(t.year)}</td><td><button onclick='showMetadata(${JSON.stringify(t.content_hash)})'>View</button></td></tr>`).join("");
    }
    async function showMetadata(hash) {
      const data = await api("/api/track?hash=" + encodeURIComponent(hash));
      document.getElementById("metadata").textContent = JSON.stringify(data.track, null, 2);
    }
    async function importMusic() {
      const path = document.getElementById("importPath").value;
      status("Importing...");
      const data = await api("/api/import", {method:"POST", body: JSON.stringify({path})});
      status(`Imported ${data.tracks} tracks, ${data.errors} errors`);
      await refreshAll();
    }
    async function refreshPlaylists() {
      const data = await api("/api/playlists");
      document.getElementById("playlistList").innerHTML = "<h2>Playlists</h2>" + data.playlists.map(p => `<div class="row"><span>${esc(p.name)}</span><span>${esc(p.track_count)} tracks</span></div>`).join("");
    }
    async function savePlaylist() {
      const name = document.getElementById("playlistName").value;
      const tracks = document.getElementById("playlistTracks").value.split("\\n").map(x => x.trim()).filter(Boolean);
      await api("/api/playlists", {method:"POST", body: JSON.stringify({name, tracks})});
      await refreshPlaylists();
    }
    async function refreshDuplicates() {
      const data = await api("/api/duplicates");
      document.getElementById("duplicatesBody").innerHTML = data.duplicates.map(d => `<tr><td>${esc(d.title)}<br><span class="muted">${esc(d.artist)}</span></td><td>${esc(d.copies)}</td><td><pre>${esc(d.paths)}</pre></td></tr>`).join("");
    }
    async function refreshThemes() {
      const data = await api("/api/themes");
      document.getElementById("themeList").innerHTML = data.themes.map(t => `<div class="row"><span>${esc(t.display_name || t.id)}</span><span>${esc(t.id)}</span></div>`).join("");
    }
    async function installTheme() {
      await api("/api/themes/install", {method:"POST", body: JSON.stringify({path: document.getElementById("themePath").value})});
      await refreshThemes();
    }
    async function refreshSettings() {
      const data = await api("/api/settings");
      document.getElementById("settingsView").textContent = JSON.stringify(data.settings, null, 2);
    }
    async function saveSettings() {
      const settings = {
        "wifi.ssid": wifiSsid.value,
        "bluetooth.headphones": btName.value,
        "audio.crossfade_seconds": crossfade.value,
        "audio.replaygain": replaygain.value,
        "audio.volume_limit": volumeLimit.value,
        "recording.quality": recordingQuality.value
      };
      await api("/api/settings", {method:"POST", body: JSON.stringify({settings})});
      await refreshSettings();
    }
    async function registerFirmware() {
      const data = await api("/api/firmware/register", {method:"POST", body: JSON.stringify({version: firmwareVersion.value, path: firmwarePath.value})});
      status("Firmware registered SHA256 " + data.sha256);
      await refreshFirmware();
    }
    async function stageFirmware() {
      const data = await api("/api/firmware/stage", {method:"POST", body: JSON.stringify({version: firmwareVersion.value, device: firmwareDevice.value})});
      status("Firmware staged: " + data.path);
    }
    async function refreshFirmware() {
      const data = await api("/api/firmware");
      document.getElementById("firmwareList").textContent = JSON.stringify(data.firmware, null, 2);
    }
    async function addVoice() {
      await api("/api/voice-notes", {method:"POST", body: JSON.stringify({title: voiceTitle.value, path: voicePath.value, linked_type: voiceType.value, linked_id: voiceLinked.value})});
      await refreshVoice();
    }
    async function refreshVoice() {
      const data = await api("/api/voice-notes");
      document.getElementById("voiceList").innerHTML = data.voice_notes.map(v => `<tr><td>${esc(v.title)}</td><td>${esc(v.linked_type)}: ${esc(v.linked_id)}</td><td>${esc(v.audio_format)}</td><td>${esc(v.audio_path)}</td></tr>`).join("");
    }
    async function previewSync() {
      const data = await api("/api/sync/preview", {method:"POST", body: JSON.stringify({path: devicePath.value})});
      syncPreview.textContent = JSON.stringify(data, null, 2);
    }
    async function syncDevice() {
      const data = await api("/api/sync", {method:"POST", body: JSON.stringify({path: devicePath.value})});
      syncPreview.textContent = JSON.stringify(data, null, 2);
    }
    async function refreshAll() {
      await Promise.all([refreshStats(), refreshTracks(), refreshPlaylists(), refreshDuplicates(), refreshThemes(), refreshSettings(), refreshFirmware(), refreshVoice()]);
    }
    refreshAll().catch(error => status(error.message));
  </script>
</body>
</html>
"""


def row_to_dict(row) -> dict:
    return {key: row[key] for key in row.keys()}


class CompanionWebServer:
    def __init__(self, core: CompanionAppCore, host: str = "127.0.0.1", port: int = 8782) -> None:
        self.core = core
        self.host = host
        self.port = port
        self.httpd: ThreadingHTTPServer | None = None

    def serve(self, open_browser: bool = True) -> None:
        core = self.core

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, format: str, *args) -> None:  # noqa: A002
                return

            def _json(self, status: int, body: dict) -> None:
                data = json.dumps(body, indent=2).encode("utf-8")
                self.send_response(status)
                self.send_header("content-type", "application/json")
                self.send_header("content-length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            def _body(self) -> dict:
                length = int(self.headers.get("content-length", "0"))
                if length <= 0:
                    return {}
                return json.loads(self.rfile.read(length).decode("utf-8"))

            def do_GET(self) -> None:
                parsed = urlparse(self.path)
                query = {key: values[0] for key, values in parse_qs(parsed.query).items()}
                try:
                    if parsed.path == "/":
                        data = HTML.encode("utf-8")
                        self.send_response(200)
                        self.send_header("content-type", "text/html; charset=utf-8")
                        self.send_header("content-length", str(len(data)))
                        self.end_headers()
                        self.wfile.write(data)
                    elif parsed.path == "/api/stats":
                        self._json(200, core.db.library_stats())
                    elif parsed.path == "/api/tracks":
                        self._json(200, {"tracks": [row_to_dict(row) for row in core.library.tracks()]})
                    elif parsed.path == "/api/track":
                        row = core.db.track_by_hash(query.get("hash", ""))
                        self._json(200, {"track": row_to_dict(row) if row else None})
                    elif parsed.path == "/api/playlists":
                        self._json(200, {"playlists": [row_to_dict(row) for row in core.db.playlists()]})
                    elif parsed.path == "/api/duplicates":
                        self._json(200, {"duplicates": [row_to_dict(row) for row in core.db.duplicates()]})
                    elif parsed.path == "/api/themes":
                        installed = core.theme.themes()
                        indexed = [row_to_dict(row) for row in core.db.themes()]
                        self._json(200, {"themes": installed + indexed})
                    elif parsed.path == "/api/settings":
                        self._json(200, {"settings": core.db.settings()})
                    elif parsed.path == "/api/firmware":
                        with core.db.connect(core.db.firmware_db) as conn:
                            rows = conn.execute("SELECT * FROM firmware_versions ORDER BY added_at DESC").fetchall()
                        self._json(200, {"firmware": [row_to_dict(row) for row in rows]})
                    elif parsed.path == "/api/voice-notes":
                        self._json(200, {"voice_notes": [row_to_dict(row) for row in core.db.voice_notes()]})
                    else:
                        self._json(404, {"error": "not_found"})
                except Exception as exc:
                    self._json(400, {"error": str(exc)})

            def do_POST(self) -> None:
                parsed = urlparse(self.path)
                try:
                    body = self._body()
                    if parsed.path == "/api/import":
                        result = core.library.import_folder(Path(body.get("path", "")).expanduser())
                        self._json(200, {"tracks": len(result.tracks), "errors": len(result.errors), "messages": result.errors})
                    elif parsed.path == "/api/playlists":
                        playlist_id = core.db.upsert_playlist(str(body.get("name", "Untitled")), list(body.get("tracks", [])))
                        self._json(200, {"id": playlist_id})
                    elif parsed.path == "/api/themes/install":
                        destination = core.theme.install_theme(Path(body.get("path", "")).expanduser())
                        manifest = json.loads((destination / "theme.json").read_text(encoding="utf-8"))
                        core.db.register_theme(str(manifest.get("id", destination.name)), str(manifest.get("display_name", destination.name)), destination / "theme.json")
                        self._json(200, {"path": str(destination)})
                    elif parsed.path == "/api/settings":
                        for key, value in dict(body.get("settings", {})).items():
                            if value != "":
                                core.db.set_setting(str(key), str(value))
                        self._json(200, {"settings": core.db.settings()})
                    elif parsed.path == "/api/firmware/register":
                        digest = core.firmware.register_firmware(str(body.get("version", "")), Path(body.get("path", "")).expanduser())
                        self._json(200, {"sha256": digest})
                    elif parsed.path == "/api/firmware/stage":
                        staged = core.firmware.stage_update(str(body.get("version", "")), Path(body.get("device", "")).expanduser())
                        self._json(200, {"path": str(staged)})
                    elif parsed.path == "/api/voice-notes":
                        note_id = core.db.add_voice_note(
                            str(body.get("title", "Voice Memory")),
                            Path(body.get("path", "")).expanduser(),
                            str(body.get("linked_type", "track")),
                            str(body.get("linked_id", "")),
                        )
                        self._json(200, {"id": note_id})
                    elif parsed.path == "/api/sync/preview":
                        plan = core.sync.plan(Path(body.get("path", "")).expanduser())
                        self._json(200, {"files_to_copy": len(plan.items), "bytes": plan.total_bytes, "already_current": plan.skipped_duplicates, "items": [{"source": str(item.source_path), "destination": str(item.destination_path), "bytes": item.bytes_to_copy} for item in plan.items[:200]]})
                    elif parsed.path == "/api/sync":
                        plan = core.sync.execute(Path(body.get("path", "")).expanduser())
                        self._json(200, {"copied_files": len(plan.items), "bytes": plan.total_bytes})
                    else:
                        self._json(404, {"error": "not_found"})
                except Exception as exc:
                    self._json(400, {"error": str(exc)})

        self.httpd = ThreadingHTTPServer((self.host, self.port), Handler)
        url = f"http://{self.host}:{self.port}/"
        print(f"[SHAeR Companion] {url}")
        if open_browser:
            threading.Timer(0.25, lambda: webbrowser.open(url)).start()
        self.httpd.serve_forever()
