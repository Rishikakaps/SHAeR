export class ShaerApiError extends Error {
  constructor(message, { kind = "api", status = 0, code = "request_failed", details = null } = {}) {
    super(message);
    this.name = "ShaerApiError";
    this.kind = kind;
    this.status = status;
    this.code = code;
    this.details = details;
  }
}

export function normalizeBaseUrl(value) {
  const candidate = String(value || "").trim().replace(/\/+$/, "");
  if (!candidate) return "";
  const withScheme = /^https?:\/\//i.test(candidate) ? candidate : `http://${candidate}`;
  const parsed = new URL(withScheme);
  if (!/^https?:$/.test(parsed.protocol)) throw new Error("SHAeR address must use HTTP or HTTPS.");
  return parsed.origin + (parsed.pathname === "/" ? "" : parsed.pathname.replace(/\/+$/, ""));
}

export class ShaerApiClient {
  constructor({ baseUrl = "", tokenProvider = async () => "", fetchImpl = globalThis.fetch } = {}) {
    this.baseUrl = normalizeBaseUrl(baseUrl);
    this.tokenProvider = tokenProvider;
    this.fetchImpl = fetchImpl;
    this.controllers = new Map();
  }

  setBaseUrl(value) {
    this.baseUrl = normalizeBaseUrl(value);
  }

  url(path) {
    if (/^https?:\/\//i.test(path)) return path;
    return `${this.baseUrl}${path.startsWith("/") ? path : `/${path}`}`;
  }

  async request(path, options = {}) {
    const headers = { Accept: "application/json", ...(options.headers || {}) };
    const token = await this.tokenProvider();
    if (token) headers.Authorization = `Bearer ${token}`;
    let body = options.body;
    if (body != null && typeof body !== "string" && !(body instanceof FormData)) {
      headers["Content-Type"] = "application/json";
      body = JSON.stringify(body);
    }

    let response;
    try {
      response = await this.fetchImpl(this.url(path), { cache: "no-store", ...options, body, headers });
    } catch (cause) {
      if (cause?.name === "AbortError") throw new ShaerApiError("Request cancelled.", { kind: "cancelled", code: "request_cancelled" });
      throw new ShaerApiError("SHAeR is unavailable on the local network.", { kind: "network", code: "device_unreachable", details: cause?.message });
    }

    const raw = await response.text();
    let payload = null;
    if (raw.trim()) {
      try {
        payload = JSON.parse(raw);
      } catch {
        throw new ShaerApiError("SHAeR returned an unreadable response.", { kind: "parse", status: response.status, code: "invalid_response" });
      }
    }
    if (!response.ok || payload?.ok === false) {
      const detail = payload?.error;
      const message = typeof detail === "string" ? detail : detail?.message;
      throw new ShaerApiError(message || `SHAeR request failed (${response.status}).`, {
        kind: response.status === 401 ? "authentication" : "http",
        status: response.status,
        code: detail?.code || (response.status === 401 ? "authentication_required" : "http_error"),
        details: detail
      });
    }
    if (payload == null) return null;
    return payload.data === undefined ? payload : payload.data;
  }

  async latest(key, path, options = {}) {
    this.controllers.get(key)?.abort();
    const controller = new AbortController();
    this.controllers.set(key, controller);
    try {
      return await this.request(path, { ...options, signal: controller.signal });
    } finally {
      if (this.controllers.get(key) === controller) this.controllers.delete(key);
    }
  }

  discovery() { return this.request("/api/v1/device/discovery"); }
  dashboard() { return this.latest("dashboard", "/api/v1/dashboard"); }
  pairingStart(deviceName) { return this.request("/api/v1/pairing/start", { method: "POST", body: { device_name: deviceName } }); }
  pairingStatus(pairingId) { return this.request(`/api/v1/pairing/status?pairing_id=${encodeURIComponent(pairingId)}`); }
  linkedDevices() { return this.request("/api/v1/pairing/trusted"); }
  revokeDevice(deviceId) { return this.request(`/api/v1/pairing/trusted/${encodeURIComponent(deviceId)}`, { method: "DELETE" }); }
  spotifyStatus() { return this.request("/api/v1/spotify/status"); }
  spotifyPlayback() { return this.latest("playback", "/api/v1/spotify/playback"); }
  spotifyQueue() { return this.latest("queue", "/api/v1/spotify/queue"); }
  spotifySaved(limit = 25, offset = 0) { return this.latest("saved", `/api/v1/spotify/library/tracks?limit=${limit}&offset=${offset}`); }
  spotifyPlaylists(limit = 25, offset = 0) { return this.latest("playlists", `/api/v1/spotify/library/playlists?limit=${limit}&offset=${offset}`); }
  spotifyRecent(limit = 20) { return this.latest("recent", `/api/v1/spotify/library/recent?limit=${limit}`); }
  spotifySearch(query, limit = 10) { return this.latest("search", `/api/v1/spotify/search?q=${encodeURIComponent(query)}&limit=${limit}`); }
  spotifyControl(action, payload = {}) { return this.request("/api/v1/spotify/control", { method: "POST", body: { action, ...payload } }); }
}
