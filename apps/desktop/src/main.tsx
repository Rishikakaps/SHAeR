import React, { useEffect, useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import {
  Archive,
  Bluetooth,
  CheckCircle2,
  ClipboardList,
  Disc3,
  Download,
  HardDrive,
  HeartHandshake,
  Library,
  Mic,
  Music,
  Play,
  Radio,
  RefreshCw,
  Rewind,
  Save,
  Search,
  Settings,
  ShieldCheck,
  SkipBack,
  SkipForward,
  Upload,
  Wrench,
  Wifi
} from "lucide-react";
import { ShaerApiClient } from "./core/api";
import type { StoredCredential } from "./core/api";
import { loadCredential, saveCredential } from "./core/credentialStore";
import { createMockSnapshot } from "./core/mockDevice";
import { ShaerDesktopError, toUserFacingError } from "./core/errors";
import { assertSupportedAudio, formatBytes, normalizeBaseUrl, sanitizeFilename } from "./core/validation";
import "./styles/app.css";

type Mode = "real" | "mock";
type View = "dashboard" | "music" | "archive" | "recordings" | "themes" | "settings" | "feedback" | "access" | "diagnostics" | "updates" | "backup" | "developer";
type TransferState = "idle" | "preparing" | "uploading" | "verifying" | "complete" | "failed";

const client = new ShaerApiClient();
const views: Array<{ id: View; label: string; icon: React.ReactNode }> = [
  { id: "dashboard", label: "Device", icon: <Disc3 size={18} /> },
  { id: "music", label: "Music", icon: <Music size={18} /> },
  { id: "archive", label: "Marginalia", icon: <Archive size={18} /> },
  { id: "recordings", label: "Recordings", icon: <Mic size={18} /> },
  { id: "themes", label: "Themes", icon: <Library size={18} /> },
  { id: "settings", label: "Settings", icon: <Settings size={18} /> },
  { id: "feedback", label: "Feedback", icon: <ClipboardList size={18} /> },
  { id: "access", label: "Linked devices", icon: <ShieldCheck size={18} /> },
  { id: "diagnostics", label: "Diagnostics", icon: <CheckCircle2 size={18} /> },
  { id: "updates", label: "Updates", icon: <Download size={18} /> },
  { id: "backup", label: "Backup", icon: <HardDrive size={18} /> },
  { id: "developer", label: "Developer", icon: <Wrench size={18} /> }
];

function getSetting(settings: any, path: string, fallback: any = "") {
  return path.split(".").reduce((cursor, part) => cursor && cursor[part], settings?.data || settings || {}) ?? fallback;
}

function listOf(payload: any, keys: string[]) {
  for (const key of keys) {
    if (Array.isArray(payload?.[key])) return payload[key];
  }
  return Array.isArray(payload) ? payload : [];
}

function App() {
  const [mode, setMode] = useState<Mode>("real");
  const [activeView, setActiveView] = useState<View>("dashboard");
  const [address, setAddress] = useState("http://shaer.local:8775");
  const [credential, setCredential] = useState<StoredCredential | null>(null);
  const [connection, setConnection] = useState("Finding device");
  const [dashboard, setDashboard] = useState<any>(null);
  const [settingsData, setSettingsData] = useState<any>(null);
  const [capabilities, setCapabilities] = useState<any>(null);
  const [wifi, setWifi] = useState<any>(null);
  const [wifiScan, setWifiScan] = useState<any>(null);
  const [bluetooth, setBluetooth] = useState<any>(null);
  const [bluetoothScan, setBluetoothScan] = useState<any>(null);
  const [branding, setBranding] = useState<any>(null);
  const [tracks, setTracks] = useState<any[]>([]);
  const [playlists, setPlaylists] = useState<any[]>([]);
  const [recordings, setRecordings] = useState<any[]>([]);
  const [archiveEntries, setArchiveEntries] = useState<any[]>([]);
  const [themes, setThemes] = useState<any[]>([]);
  const [linkedDevices, setLinkedDevices] = useState<any[]>([]);
  const [diagnostics, setDiagnostics] = useState<any[]>([]);
  const [feedbackReports, setFeedbackReports] = useState<any[]>([]);
  const [developerDashboard, setDeveloperDashboard] = useState<any>(null);
  const [feedbackMessage, setFeedbackMessage] = useState("");
  const [feedbackState, setFeedbackState] = useState("Ready");
  const [releaseDraft, setReleaseDraft] = useState({ kind: "os", version: "", title: "I learned something new. Ready to update?", notes: "", url: "" });
  const [updateStatus, setUpdateStatus] = useState<any>(null);
  const [backupStatus, setBackupStatus] = useState<any>(null);
  const [musicQuery, setMusicQuery] = useState("");
  const [spotifyView, setSpotifyView] = useState("saved");
  const [error, setError] = useState("");
  const [pairing, setPairing] = useState<any>(null);
  const [transfer, setTransfer] = useState<{ state: TransferState; detail: string }>({ state: "idle", detail: "No transfer started." });
  const mock = useMemo(createMockSnapshot, []);

  const real = mode === "real";
  const connected = connection === "Connected" || connection === "Mock Device";
  const theme = getSetting(settingsData, "display.theme", dashboard?.current_theme || "archive_dark");
  const nowPlaying = dashboard?.now_playing || {};
  const filteredTracks = tracks.filter((track) => {
    const haystack = [track.title, track.artist, track.album, track.filepath].join(" ").toLowerCase();
    return haystack.includes(musicQuery.toLowerCase());
  });

  useEffect(() => {
    loadCredential().then(async (saved) => {
      if (!saved) return;
      setCredential(saved);
      setAddress(saved.baseUrl);
      client.setSession(saved.baseUrl, saved.token);
      await refreshReal("Reconnect");
    });
  }, []);

  useEffect(() => {
    if (!connected) return;
    loadView(activeView);
  }, [activeView, connected]);

  async function refreshReal(label = "Refresh") {
    setError("");
    setConnection(`${label}...`);
    try {
      const [nextDashboard, nextSettings, nextCapabilities, nextBranding] = await Promise.all([
        client.dashboard(),
        client.settings(),
        client.capabilities().catch(() => ({ capabilities: {} })),
        client.branding().catch(() => null)
      ]);
      setDashboard(nextDashboard);
      setSettingsData(nextSettings);
      setCapabilities(nextCapabilities);
      setBranding(nextBranding);
      setConnection("Connected");
    } catch (err) {
      setConnection(err instanceof ShaerDesktopError && err.status === 401 ? "Pairing required" : "Offline");
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function loadView(view: View) {
    if (mode === "mock") return;
    setError("");
    try {
      if (view === "music") {
        const [music, playlistData] = await Promise.all([client.music(), client.playlists().catch(() => ({ playlists: [] }))]);
        setTracks(listOf(music, ["tracks"]));
        setPlaylists(listOf(playlistData, ["playlists"]));
      }
      if (view === "recordings") setRecordings(listOf(await client.recordings(), ["recordings", "items"]));
      if (view === "archive") setArchiveEntries(listOf(await client.archive(), ["entries", "archive", "items"]));
      if (view === "themes") setThemes(listOf(await client.themes(), ["themes"]));
      if (view === "access") setLinkedDevices(listOf(await client.linkedDevices(), ["devices", "companions"]));
      if (view === "diagnostics") setDiagnostics(listOf(await client.diagnostics(), ["diagnostics", "checks"]));
      if (view === "feedback") setFeedbackReports(listOf(await client.feedback(), ["reports"]));
      if (view === "developer") {
        const dashboard = await client.developerDashboard();
        setDeveloperDashboard(dashboard);
        setFeedbackReports(listOf(dashboard, ["feedback"]));
      }
      if (view === "updates") setUpdateStatus(await client.updateStatus());
      if (view === "backup") setBackupStatus(await client.backupStatus());
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function loadConnectivity() {
    setError("");
    try {
      const [wifiStatus, wifiNetworks, btStatus, btDevices] = await Promise.all([
        client.wifiStatus(),
        client.wifiScan(),
        client.bluetoothStatus(),
        client.bluetoothScan()
      ]);
      setWifi(wifiStatus);
      setWifiScan(wifiNetworks);
      setBluetooth(btStatus);
      setBluetoothScan(btDevices);
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function manualConnect() {
    setMode("real");
    setError("");
    try {
      const baseUrl = normalizeBaseUrl(address);
      client.setSession(baseUrl, credential?.token || "");
      const discovered = await client.discovery(baseUrl);
      setDashboard({ device_name: discovered.device_name, firmware_version: discovered.firmware_version });
      setConnection(credential?.token ? "Checking pairing..." : "SHAeR found, pair this computer");
      if (credential?.token) await refreshReal("Connect");
    } catch (err) {
      setConnection("Offline");
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function startPairing() {
    setError("");
    try {
      client.setSession(address, "");
      const started = await client.startPairing("SHAeR Desktop Companion");
      setPairing(started);
      pollPairing(started.pairing_id);
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function pollPairing(pairingId: string) {
    for (let attempt = 0; attempt < 120; attempt += 1) {
      await new Promise((resolve) => window.setTimeout(resolve, 1000));
      const status = await client.pairingStatus(pairingId);
      setPairing(status);
      if (status.state === "paired" && status.token) {
        const next = {
          baseUrl: normalizeBaseUrl(address),
          token: status.token,
          deviceId: status.device?.id || "",
          deviceName: status.device?.name || dashboard?.device_name || "SHAeR"
        };
        await saveCredential(next);
        setCredential(next);
        client.setSession(next.baseUrl, next.token);
        await refreshReal("Pair");
        return;
      }
      if (["denied", "expired", "cancelled"].includes(status.state)) return;
    }
  }

  async function saveSetting(path: string, value: unknown, readBack: (settings: any) => unknown) {
    setError("");
    try {
      await client.updateSettings({ [path]: value });
      const next = await client.settings();
      setSettingsData(next);
      if (String(readBack(next)) !== String(value)) {
        throw new ShaerDesktopError("SHAeR accepted the request, but read-back did not match.", { code: "readback_mismatch", dataChanged: true });
      }
      await refreshReal("Verify");
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function applyTheme(themeId: string) {
    setError("");
    try {
      await client.setTheme(themeId);
      await refreshReal("Theme read-back");
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function control(action: string) {
    try {
      await client.playbackControl(action);
      await refreshReal("Playback");
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  async function uploadOneFile(event: React.ChangeEvent<HTMLInputElement>) {
    const file = event.target.files?.[0];
    if (!file) return;
    setError("");
    try {
      setTransfer({ state: "preparing", detail: `Validating ${file.name}` });
      assertSupportedAudio(file.name, ["mp3"]);
      if (file.size > 90 * 1024 * 1024) throw new Error("This first vertical slice only accepts files up to 90 MB.");
      const before = await client.music();
      const safeName = sanitizeFilename(file.name);
      setTransfer({ state: "uploading", detail: `Uploading ${safeName}` });
      await client.uploadMusic(safeName, await fileToBase64(file));
      setTransfer({ state: "verifying", detail: "Reading SHAeR library after upload." });
      const after = await client.music();
      const existedBefore = new Set((before.tracks || []).map((track: any) => String(track.filepath || track.title || "")));
      const match = (after.tracks || []).find((track: any) => !existedBefore.has(String(track.filepath || track.title || "")));
      if (!match) throw new Error("Upload finished, but the track did not appear in SHAeR's library read-back.");
      setTracks(after.tracks || []);
      setTransfer({ state: "complete", detail: `Verified in library: ${match.title || safeName}` });
    } catch (err) {
      const message = toUserFacingError(err).whatHappened;
      setTransfer({ state: "failed", detail: message });
      setError(message);
    }
  }

  async function uploadAppIcon(event: React.ChangeEvent<HTMLInputElement>) {
    const file = event.target.files?.[0];
    if (!file) return;
    setError("");
    try {
      if (file.type && file.type !== "image/png") throw new Error("Use a PNG icon for synced SHAeR branding.");
      if (file.size > 4 * 1024 * 1024) throw new Error("Icon PNG must be 4 MB or smaller.");
      await client.uploadAppIcon(await fileToBase64(file));
      setBranding(await client.branding());
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  function enableMock() {
    if (!import.meta.env.DEV) return;
    setMode("mock");
    setConnection("Mock Device");
    setDashboard(mock.dashboard);
    setSettingsData(mock.settings);
    setWifi(mock.wifi);
    setWifiScan(mock.wifiScan);
    setBluetooth(mock.bluetooth);
    setBluetoothScan(mock.bluetoothScan);
    setCapabilities({ capabilities: { mock: true } });
    setBranding({ app_name: "SHAeR", icon_url: "", custom_icon: false });
    setTracks([{ id: "mock", title: "Test Pressing", artist: "SHAeR", album: "Companion", duration_s: 181 }]);
    setPlaylists([{ id: "mock-list", name: "On SHAeR", track_count: 1 }]);
    setRecordings([{ id: "mock-rec", title: "Voice note", created_at: "Today", duration_s: 42 }]);
    setArchiveEntries([{ id: "mock-archive", title: "Margin page", summary: "Linked to the current track." }]);
    setThemes([{ id: theme, name: "Active theme", description: "Rendered on SHAeR" }]);
    setLinkedDevices([{ id: "mock-device", name: "Desktop Companion", state: "trusted" }]);
    setDiagnostics([{ name: "Contract tests", status: "mock" }]);
    setFeedbackReports([{ id: "mock-feedback", message: "Bluetooth disconnected while changing playlists.", created_at: Date.now() / 1000, context: { theme, companion_version: "desktop-mock" } }]);
    setDeveloperDashboard({ feedback: feedbackReports, releases: [], device: mock.dashboard, updates: { state: "mock" } });
    setUpdateStatus({ state: "Idle" });
    setBackupStatus({ state: "Ready" });
  }

  function supportContext() {
    return {
      shaer_os_version: dashboard?.firmware_version || dashboard?.shaer_os_version || "unknown",
      theme,
      device_serial: dashboard?.device_id || credential?.deviceId || "unknown",
      battery_percent: dashboard?.battery_percent ?? null,
      storage_free: dashboard?.storage?.free ?? null,
      spotify_connected: Boolean(dashboard?.spotify_authenticated),
      bluetooth_connected: Boolean(bluetooth?.active_device),
      wifi_ssid: wifi?.ssid || "unknown",
      companion_version: "desktop-0.1.0",
      capability_snapshot: capabilities?.capabilities || capabilities || {},
      connection
    };
  }

  async function sendFeedback() {
    setFeedbackState("Sending...");
    try {
      await client.sendFeedback({ message: feedbackMessage, severity: "note", context: supportContext() });
      setFeedbackMessage("");
      setFeedbackState("Sent to SHAeR");
      setFeedbackReports(listOf(await client.feedback(), ["reports"]));
    } catch (err) {
      setFeedbackState(toUserFacingError(err).whatHappened);
    }
  }

  async function createRelease() {
    try {
      await client.createDeveloperRelease(releaseDraft);
      setDeveloperDashboard(await client.developerDashboard());
      setReleaseDraft({ kind: releaseDraft.kind, version: "", title: "A new chapter for SHAeR is ready.", notes: "", url: "" });
    } catch (err) {
      setError(toUserFacingError(err).whatHappened);
    }
  }

  return (
    <main className={mode === "mock" ? "app mock" : "app"}>
      {mode === "mock" && <div className="mock-banner">MOCK DEVICE - development only. No real SHAeR operation is being performed.</div>}
      <header className="topbar">
        <button className="brand" onClick={() => setActiveView("dashboard")}>
          {branding?.icon_url && <img src={client.baseUrl ? `${client.baseUrl}${branding.icon_url}` : branding.icon_url} alt="" />}
          <span>SHAeR</span>
        </button>
        <div className={`connection ${connected ? "online" : ""}`}><i /><span>{connection}</span></div>
        <div className="address-bar">
          <input value={address} onChange={(event) => setAddress(event.target.value)} placeholder="shaer.local:8775" />
          <button onClick={manualConnect} title="Connect"><Radio size={16} />Connect</button>
          <button onClick={startPairing} title="Pair"><HeartHandshake size={16} />Pair</button>
          <button onClick={() => refreshReal("Refresh")} disabled={!real || !credential} title="Refresh"><RefreshCw size={16} /></button>
          {import.meta.env.DEV && <button className="danger" onClick={enableMock}>Mock</button>}
        </div>
      </header>

      <aside className="sidebar">
        <nav>
          {views.map((view) => <button key={view.id} className={activeView === view.id ? "active" : ""} onClick={() => setActiveView(view.id)}>{view.icon}<span>{view.label}</span></button>)}
        </nav>
        <div className="device-mini"><strong>{dashboard?.device_name || "SHAeR"}</strong><span>{dashboard?.firmware_version || dashboard?.shaer_os_version || "Not paired"}</span></div>
      </aside>

      <section className="workspace">
        {error && <div className="error">{error}</div>}
        {pairing && <div className="app-state"><strong>Pairing</strong><span>{pairing.state || "requested"} {pairing.code ? `Code ${pairing.code}` : ""}</span></div>}
        {activeView === "dashboard" && <Dashboard dashboard={dashboard} nowPlaying={nowPlaying} theme={theme} onControl={control} onConnectivity={loadConnectivity} wifi={wifi} wifiScan={wifiScan} bluetooth={bluetooth} bluetoothScan={bluetoothScan} />}
        {activeView === "music" && <MusicView tracks={filteredTracks} playlists={playlists} query={musicQuery} setQuery={setMusicQuery} spotifyView={spotifyView} setSpotifyView={setSpotifyView} transfer={transfer} uploadOneFile={uploadOneFile} />}
        {activeView === "archive" && <ListView eyebrow="Musical Memory" title="Marginalia" empty="Pair a SHAeR to browse its archive." items={archiveEntries} />}
        {activeView === "recordings" && <ListView eyebrow="Personal Archive" title="Recordings" empty="Pair a SHAeR to browse recordings." items={recordings} />}
        {activeView === "themes" && <ThemesView themes={themes} activeTheme={theme} onApply={applyTheme} />}
        {activeView === "settings" && <SettingsView settingsData={settingsData} dashboard={dashboard} theme={theme} saveSetting={saveSetting} applyTheme={applyTheme} uploadAppIcon={uploadAppIcon} branding={branding} connected={connected} />}
        {activeView === "feedback" && <FeedbackView message={feedbackMessage} setMessage={setFeedbackMessage} state={feedbackState} onSend={sendFeedback} context={supportContext()} reports={feedbackReports} />}
        {activeView === "access" && <ListView eyebrow="Security" title="Linked devices" empty="Connect to inspect companion access." items={linkedDevices} />}
        {activeView === "diagnostics" && <DiagnosticsView diagnostics={diagnostics} capabilities={capabilities} onRun={(name: string) => client.runDiagnostic(name).then(() => loadView("diagnostics")).catch((err) => setError(toUserFacingError(err).whatHappened))} />}
        {activeView === "updates" && <StatusView eyebrow="System" title="Firmware updates" status={updateStatus} actionLabel="Refresh update status" onAction={() => loadView("updates")} />}
        {activeView === "backup" && <StatusView eyebrow="Recovery" title="Backup & restore" status={backupStatus} actionLabel="Refresh backup status" onAction={() => loadView("backup")} />}
        {activeView === "developer" && <DeveloperView dashboard={developerDashboard} releaseDraft={releaseDraft} setReleaseDraft={setReleaseDraft} onCreateRelease={createRelease} />}
      </section>
    </main>
  );
}

function Dashboard({ dashboard, nowPlaying, theme, onControl, onConnectivity, wifi, wifiScan, bluetooth, bluetoothScan }: any) {
  const storage = dashboard?.storage || {};
  return (
    <>
      <SectionHeading eyebrow="Device" title={dashboard?.device_name || "Overview"} pill={dashboard?.spotify_authenticated ? "Spotify connected" : "Spotify unknown"} />
      <section className="now-playing panel">
        <div className="artwork">{nowPlaying.cover_art ? <img src={nowPlaying.cover_art} alt="Album artwork" /> : <span>SHAeR</span>}</div>
        <div className="track-copy">
          <span className="eyebrow">{String(nowPlaying.source || "Idle").toUpperCase()}</span>
          <h2>{nowPlaying.title || "Nothing playing"}</h2>
          <p>{[nowPlaying.artist, nowPlaying.album].filter(Boolean).join(" - ") || "Choose music on SHAeR"}</p>
          <div className="playback-actions">
            <button onClick={() => onControl("previous")} title="Previous"><SkipBack size={18} /></button>
            <button className="primary-control" onClick={() => onControl("play_pause")} title="Play or pause"><Play size={19} /></button>
            <button onClick={() => onControl("next")} title="Next"><SkipForward size={18} /></button>
            <button onClick={() => onControl("volume_down")} title="Volume down">-</button>
            <button onClick={() => onControl("volume_up")} title="Volume up">+</button>
          </div>
        </div>
      </section>
      <div className="metric-grid">
        <Metric label="Battery" value={dashboard?.battery_percent == null ? "Unavailable" : `${dashboard.battery_percent}%`} detail={dashboard?.charging ? "Charging" : "Hardware pending"} />
        <Metric label="Storage" value={storage.total ? `${Math.round(storage.used / storage.total * 100)}% used` : "--"} detail={storage.free ? `${formatBytes(storage.free)} available` : "Waiting for device"} />
        <Metric label="Theme" value={theme} detail="Rendered on SHAeR" />
        <Metric label="System" value={dashboard?.cpu_temperature_c == null ? "Unavailable" : `${dashboard.cpu_temperature_c}°C`} detail={`Uptime ${dashboard?.uptime_s || "--"}`} />
      </div>
      <section className="panel connectivity-panel">
        <div className="panel-title"><h2>Connectivity</h2><button onClick={onConnectivity}><Wifi size={16} />Read from SHAeR</button></div>
        <div className="split-layout">
          <DevicePane icon={<Wifi size={17} />} title="Wi-Fi" lines={[`SSID: ${wifi?.ssid || "Unknown"}`, `IP: ${wifi?.ip_address || "Unknown"}`, `Hostname: ${wifi?.hostname || "Unknown"}`]} items={(wifiScan?.networks || []).map((n: any) => `${n.ssid}${n.current ? " (current)" : ""}`)} />
          <DevicePane icon={<Bluetooth size={17} />} title="Bluetooth" lines={[`Enabled: ${String(Boolean(bluetooth?.enabled))}`, `Discoverable: ${String(Boolean(bluetooth?.discoverable))}`]} items={(bluetoothScan?.devices || bluetooth?.paired_devices || []).map((d: any) => `${d.name}${d.state ? ` (${d.state})` : ""}`)} />
        </div>
      </section>
    </>
  );
}

function MusicView({ tracks, playlists, query, setQuery, spotifyView, setSpotifyView, transfer, uploadOneFile }: any) {
  return (
    <>
      <SectionHeading eyebrow="Library" title="Music" action={<label className="file-action"><Upload size={16} />Add music<input type="file" accept=".mp3,audio/mpeg" onChange={uploadOneFile} /></label>} />
      <div className="segmented"><button className="active">On SHAeR</button><button>Spotify</button></div>
      <div className="toolbar"><Search size={17} /><input value={query} onChange={(event) => setQuery(event.target.value)} placeholder="Search tracks, artists, albums" /><button>New playlist</button></div>
      <div className="split-layout">
        <section className="panel table-panel">
          <div className="table-header"><span>Track</span><span>Album</span><span>Length</span></div>
          {tracks.length ? tracks.map((track: any) => <div className="track-row" key={track.id || track.filepath || track.title}><div><strong>{track.title || track.filepath || "Untitled"}</strong><small>{track.artist || "Unknown artist"}</small></div><span>{track.album || "--"}</span><span>{track.duration_s ? `${Math.round(track.duration_s / 60)}m` : "--"}</span></div>) : <div className="empty-state">Pair a SHAeR to browse its library.</div>}
        </section>
        <aside className="panel playlist-panel">
          <h2>Playlists</h2>
          {playlists.length ? playlists.map((playlist: any) => <button className="playlist-item" key={playlist.id || playlist.name}><span>{playlist.name}</span><small>{playlist.track_count || 0} tracks</small></button>) : <div className="empty-state">No playlists loaded.</div>}
          <p className={`transfer ${transfer.state}`}>{transfer.state}: {transfer.detail}</p>
        </aside>
      </div>
      <section className="panel spotify-panel">
        <div className="panel-title"><h2>Spotify</h2><span>Real account data after authentication</span></div>
        <div className="segmented compact">{["saved", "playlists", "recent", "queue", "search"].map((view) => <button key={view} className={spotifyView === view ? "active" : ""} onClick={() => setSpotifyView(view)}>{view}</button>)}</div>
        <div className="empty-state">Spotify collection view is matched to the phone app and loads from SHAeR when the device exposes this endpoint.</div>
      </section>
    </>
  );
}

function SettingsView({ settingsData, dashboard, theme, saveSetting, applyTheme, uploadAppIcon, branding, connected }: any) {
  return (
    <>
      <SectionHeading eyebrow="Device" title="Settings" pill="Changes sync immediately" />
      <section className="settings-groups">
        <div className="setting-group">
          <h2>Display</h2>
          <Setting label="Brightness" value={getSetting(settingsData, "display.brightness", "")} type="number" onSave={(value) => saveSetting("display.brightness", Number(value), (s: any) => getSetting(s, "display.brightness"))} />
          <Setting label="Sleep timeout seconds" value={getSetting(settingsData, "display.sleep_timeout_s", "")} type="number" onSave={(value) => saveSetting("display.sleep_timeout_s", Number(value), (s: any) => getSetting(s, "display.sleep_timeout_s"))} />
          <Setting label="Show battery percentage" value={String(getSetting(settingsData, "display.show_battery_percent", false))} onSave={(value) => saveSetting("display.show_battery_percent", value === "true", (s: any) => getSetting(s, "display.show_battery_percent"))} />
        </div>
        <div className="setting-group">
          <h2>Device</h2>
          <Setting label="Device name" value={getSetting(settingsData, "device.name", dashboard?.device_name || "")} onSave={(value) => saveSetting("device.name", value, (s: any) => getSetting(s, "device.name"))} />
          <div className="setting-row"><label>Active theme</label><div className="inline-control"><input value={theme} readOnly /><button onClick={() => applyTheme(theme)}><Save size={16} />Verify</button></div></div>
        </div>
        <div className="setting-group">
          <h2>Synced app icon</h2>
          <div className="setting-row">
            <label>{branding?.custom_icon ? "Custom icon synced" : "Default icon"}</label>
            <div className="inline-control">
              {branding?.icon_url && <img className="app-icon-preview" src={client.baseUrl ? `${client.baseUrl}${branding.icon_url}` : branding.icon_url} alt="Current SHAeR app icon" />}
              <label className="file-action"><Upload size={16} />PNG<input type="file" accept="image/png,.png" onChange={uploadAppIcon} disabled={!connected} /></label>
            </div>
          </div>
        </div>
      </section>
    </>
  );
}

function ThemesView({ themes, activeTheme, onApply }: any) {
  return (
    <>
      <SectionHeading eyebrow="Appearance" title="Themes" />
      <div className="theme-grid">
        {themes.length ? themes.map((theme: any) => <article className={`theme-card ${theme.id === activeTheme ? "active" : ""}`} key={theme.id || theme.name}><div className="theme-preview" /><footer><div><strong>{theme.name || theme.id}</strong><small>{theme.description || "SHAeR visual theme"}</small></div><button onClick={() => onApply(theme.id)}>Apply</button></footer></article>) : <div className="empty-state">Pair a SHAeR to manage themes.</div>}
      </div>
    </>
  );
}

function DiagnosticsView({ diagnostics, capabilities, onRun }: any) {
  return (
    <>
      <SectionHeading eyebrow="Health" title="Diagnostics" />
      <div className="diagnostic-list">
        {diagnostics.length ? diagnostics.map((item: any) => <div className="diagnostic-item" key={item.name || item.id}><strong>{item.name || item.id}</strong><span>{item.status || "ready"}</span><button onClick={() => onRun(item.name || item.id)}>Run</button></div>) : <div className="empty-state">Pair a SHAeR to inspect diagnostics.</div>}
      </div>
      <pre className="terminal-output">{JSON.stringify(capabilities?.capabilities || capabilities || {}, null, 2)}</pre>
    </>
  );
}

function StatusView({ eyebrow, title, status, actionLabel, onAction }: any) {
  return <><SectionHeading eyebrow={eyebrow} title={title} action={<button onClick={onAction}><RefreshCw size={16} />{actionLabel}</button>} /><section className="panel form-panel"><pre>{JSON.stringify(status || { state: "Connect to SHAeR to load this view." }, null, 2)}</pre></section></>;
}

function FeedbackView({ message, setMessage, state, onSend, context, reports }: any) {
  return (
    <>
      <SectionHeading eyebrow="Feedback" title="Tell SHAeR Something" pill={state} />
      <section className="feedback-grid">
        <div className="panel form-panel feedback-composer">
          <h2>Report an issue</h2>
          <textarea value={message} onChange={(event) => setMessage(event.target.value)} placeholder="Bluetooth disconnected while changing playlists." />
          <div className="button-row"><button onClick={onSend} disabled={!message.trim()}><Upload size={16} />Send with diagnostics</button></div>
        </div>
        <aside className="panel context-panel">
          <h2>Sent with the note</h2>
          <pre>{JSON.stringify(context, null, 2)}</pre>
        </aside>
      </section>
      <section className="feedback-list">
        {reports.length ? reports.map((report: any) => <ReportCard key={report.id || report.created_at} report={report} />) : <div className="empty-state">No feedback reports yet.</div>}
      </section>
    </>
  );
}

function DeveloperView({ dashboard, releaseDraft, setReleaseDraft, onCreateRelease }: any) {
  const releases = listOf(dashboard, ["releases"]);
  const feedback = listOf(dashboard, ["feedback"]);
  const updateDraft = (key: string, value: string) => setReleaseDraft((draft: any) => ({ ...draft, [key]: value }));
  return (
    <>
      <SectionHeading eyebrow="Developer" title="SHAeR Control Room" pill={`${feedback.length} reports`} />
      <section className="developer-grid">
        <div className="panel form-panel release-composer">
          <h2>Publish an update notice</h2>
          <div className="form-row">
            <select value={releaseDraft.kind} onChange={(event) => updateDraft("kind", event.target.value)}>
              <option value="os">OS update</option>
              <option value="theme">Theme</option>
              <option value="app">Companion app</option>
              <option value="firmware">Firmware</option>
            </select>
            <input value={releaseDraft.version} onChange={(event) => updateDraft("version", event.target.value)} placeholder="Version" />
          </div>
          <input value={releaseDraft.title} onChange={(event) => updateDraft("title", event.target.value)} placeholder="I learned something new. Ready to update?" />
          <textarea value={releaseDraft.notes} onChange={(event) => updateDraft("notes", event.target.value)} placeholder={"Added:\nNew theme\nFaster sync\nAudio improvements"} />
          <input value={releaseDraft.url} onChange={(event) => updateDraft("url", event.target.value)} placeholder="GitHub Release asset URL" />
          <div className="button-row"><button onClick={onCreateRelease} disabled={!releaseDraft.version.trim()}><Download size={16} />Create update notice</button></div>
        </div>
        <div className="developer-column">
          <section className="panel release-list">
            <h2>Ready for Okayy + Sync</h2>
            {releases.length ? releases.map((release: any) => <article className="release-card" key={release.id}><strong>{release.title}</strong><small>{release.kind} {release.version}</small><p>{release.notes || "No notes added."}</p><button disabled>Okayy + Sync</button></article>) : <div className="empty-state">No update notices published.</div>}
          </section>
          <section className="panel release-list">
            <h2>Feedback inbox</h2>
            {feedback.length ? feedback.map((report: any) => <ReportCard key={report.id || report.created_at} report={report} />) : <div className="empty-state">No reports received.</div>}
          </section>
        </div>
      </section>
    </>
  );
}

function ReportCard({ report }: any) {
  return (
    <article className="report-card">
      <div><strong>{report.message || "Untitled report"}</strong><small>{report.severity || "note"} · {formatDate(report.created_at)}</small></div>
      <pre>{JSON.stringify(report.context || {}, null, 2)}</pre>
    </article>
  );
}

function ListView({ eyebrow, title, empty, items }: any) {
  return <><SectionHeading eyebrow={eyebrow} title={title} /><div className="recording-list">{items.length ? items.map((item: any) => <article className="recording-item" key={item.id || item.name || item.title}><div className="recording-icon"><Rewind size={16} /></div><div className="recording-copy"><strong>{item.title || item.name || item.id}</strong><small>{item.summary || item.created_at || item.state || "Stored on SHAeR"}</small></div></article>) : <div className="empty-state">{empty}</div>}</div></>;
}

function SectionHeading({ eyebrow, title, pill, action }: { eyebrow: string; title: string; pill?: string; action?: React.ReactNode }) {
  return <div className="section-heading"><div><span className="eyebrow">{eyebrow}</span><h1>{title}</h1></div>{action || (pill ? <span className="status-pill">{pill}</span> : null)}</div>;
}

function Metric({ label, value, detail }: { label: string; value: string; detail: string }) {
  return <article className="metric"><span>{label}</span><strong>{value}</strong><small>{detail}</small></article>;
}

function DevicePane({ icon, title, lines, items }: { icon: React.ReactNode; title: string; lines: string[]; items: string[] }) {
  return <div><h3>{icon}{title}</h3>{lines.map((line) => <p key={line}>{line}</p>)}<ul>{items.map((item) => <li key={item}>{item}</li>)}</ul></div>;
}

function formatDate(seconds: number) {
  if (!seconds) return "just now";
  return new Intl.DateTimeFormat(undefined, { dateStyle: "medium", timeStyle: "short" }).format(new Date(Number(seconds) * 1000));
}

function Setting({ label, value, type = "text", onSave }: { label: string; value: string; type?: string; onSave: (value: string) => void }) {
  const [draft, setDraft] = useState(value);
  useEffect(() => setDraft(value), [value]);
  return <div className="setting-row"><label>{label}</label><div className="inline-control"><input type={type} value={draft} onChange={(event) => setDraft(event.target.value)} /><button onClick={() => onSave(draft)}>Save</button></div></div>;
}

function fileToBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onerror = () => reject(new Error("Could not read selected file."));
    reader.onload = () => resolve(String(reader.result || "").split(",", 2)[1] || "");
    reader.readAsDataURL(file);
  });
}

createRoot(document.getElementById("root")!).render(<App />);
