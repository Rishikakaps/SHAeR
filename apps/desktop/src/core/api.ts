import { ShaerDesktopError } from "./errors";
import { normalizeBaseUrl } from "./validation";

export interface StoredCredential {
  baseUrl: string;
  token: string;
  deviceId: string;
  deviceName: string;
}

export class ShaerApiClient {
  baseUrl = "";
  token = "";

  constructor(baseUrl = "", token = "") {
    if (baseUrl) this.baseUrl = normalizeBaseUrl(baseUrl);
    this.token = token;
  }

  setSession(baseUrl: string, token = this.token) {
    this.baseUrl = normalizeBaseUrl(baseUrl);
    this.token = token;
  }

  async request<T>(path: string, init: RequestInit = {}): Promise<T> {
    if (!this.baseUrl) throw new ShaerDesktopError("Connect to SHAeR before sending a request.", { code: "missing_device" });
    const headers = new Headers(init.headers);
    headers.set("Accept", "application/json");
    if (this.token) headers.set("Authorization", `Bearer ${this.token}`);
    let body = init.body;
    if (body && typeof body !== "string" && !(body instanceof FormData)) {
      headers.set("Content-Type", "application/json");
      body = JSON.stringify(body);
    }
    let response: Response;
    try {
      response = await fetch(`${this.baseUrl}${path}`, { ...init, body, headers, cache: "no-store" });
    } catch (error) {
      throw new ShaerDesktopError("SHAeR did not respond on the network.", { code: "device_unreachable", affected: "No device state was changed." });
    }
    const raw = await response.text();
    let payload: any = null;
    if (raw.trim()) {
      try {
        payload = JSON.parse(raw);
      } catch {
        throw new ShaerDesktopError("SHAeR returned an unreadable response.", { code: "invalid_json", status: response.status });
      }
    }
    if (!response.ok || payload?.ok === false) {
      const detail = payload?.error;
      const message = typeof detail === "string" ? detail : detail?.message;
      throw new ShaerDesktopError(message || `SHAeR request failed (${response.status}).`, {
        code: detail?.code || "request_failed",
        status: response.status,
        affected: "The requested operation did not complete."
      });
    }
    return (payload?.data ?? payload) as T;
  }

  discovery(baseUrl?: string) {
    const previous = this.baseUrl;
    if (baseUrl) this.baseUrl = normalizeBaseUrl(baseUrl);
    return this.request<any>("/api/v1/device/discovery").finally(() => {
      if (baseUrl) this.baseUrl = previous;
    });
  }

  dashboard() { return this.request<any>("/api/v1/dashboard"); }
  capabilities() { return this.request<any>("/api/v1/device/capabilities"); }
  settings() { return this.request<any>("/api/v1/settings"); }
  themes() { return this.request<any>("/api/v1/themes"); }
  wifiStatus() { return this.request<any>("/api/v1/network/wifi"); }
  wifiScan() { return this.request<any>("/api/v1/network/wifi/scan"); }
  bluetoothStatus() { return this.request<any>("/api/v1/bluetooth"); }
  bluetoothScan() { return this.request<any>("/api/v1/bluetooth/scan"); }
  music() { return this.request<any>("/api/v1/music/tracks"); }
  playlists() { return this.request<any>("/api/v1/music/playlists"); }
  recordings() { return this.request<any>("/api/v1/recordings"); }
  archive() { return this.request<any>("/api/v1/archive"); }
  linkedDevices() { return this.request<any>("/api/v1/pairing/devices"); }
  diagnostics() { return this.request<any>("/api/v1/diagnostics"); }
  updateStatus() { return this.request<any>("/api/v1/update/status"); }
  backupStatus() { return this.request<any>("/api/v1/backup/status"); }
  branding() { return this.request<any>("/api/v1/branding"); }

  updateSettings(payload: Record<string, unknown>) {
    return this.request<any>("/api/v1/settings", { method: "POST", body: payload as any });
  }

  setTheme(themeId: string) {
    return this.request<any>("/api/v1/themes/active", { method: "POST", body: { theme_id: themeId } as any });
  }

  uploadMusic(filename: string, contentBase64: string) {
    return this.request<any>("/api/v1/music/upload", { method: "POST", body: { filename, content_base64: contentBase64 } as any });
  }

  uploadAppIcon(contentBase64: string) {
    return this.request<any>("/api/v1/branding/app-icon", { method: "POST", body: { mime_type: "image/png", content_base64: contentBase64 } as any });
  }

  playbackControl(action: string) {
    return this.request<any>("/api/v1/playback/control", { method: "POST", body: { action } as any });
  }

  runDiagnostic(name: string) {
    return this.request<any>("/api/v1/diagnostics/run", { method: "POST", body: { name } as any });
  }

  createBackup(payload: Record<string, unknown>) {
    return this.request<any>("/api/v1/backup/create", { method: "POST", body: payload as any });
  }

  stageUpdate(payload: Record<string, unknown>) {
    return this.request<any>("/api/v1/update/stage", { method: "POST", body: payload as any });
  }

  startPairing(deviceName: string) {
    return this.request<any>("/api/v1/pairing/start", { method: "POST", body: { device_name: deviceName } as any });
  }

  pairingStatus(pairingId: string) {
    return this.request<any>(`/api/v1/pairing/status?pairing_id=${encodeURIComponent(pairingId)}`);
  }
}
