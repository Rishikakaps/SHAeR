export function normalizeBaseUrl(value: string): string {
  const candidate = String(value || "").trim().replace(/\/+$/, "");
  if (!candidate) throw new Error("Enter a SHAeR address.");
  if (/^[a-z][a-z0-9+.-]*:\/\//i.test(candidate) && !/^https?:\/\//i.test(candidate)) {
    throw new Error("SHAeR addresses must use HTTP or HTTPS.");
  }
  const withScheme = /^https?:\/\//i.test(candidate) ? candidate : `http://${candidate}`;
  const parsed = new URL(withScheme);
  if (!/^https?:$/.test(parsed.protocol)) throw new Error("SHAeR addresses must use HTTP or HTTPS.");
  return parsed.origin + (parsed.pathname === "/" ? "" : parsed.pathname.replace(/\/+$/, ""));
}

export function sanitizeFilename(name: string): string {
  const cleaned = name.replace(/[\\/:*?"<>|\u0000-\u001f]/g, "_").replace(/\s+/g, " ").trim();
  if (!cleaned || cleaned === "." || cleaned === ".." || !/[A-Za-z0-9]/.test(cleaned)) return "shaer-file";
  return cleaned.slice(0, 180);
}

export function assertSupportedAudio(filename: string, supportedFormats: string[]): void {
  const ext = filename.split(".").pop()?.toLowerCase() || "";
  if (!supportedFormats.map((item) => item.toLowerCase()).includes(ext)) {
    throw new Error(`SHAeR does not report support for .${ext || "unknown"} audio files.`);
  }
}

export function formatBytes(bytes = 0): string {
  if (!Number.isFinite(bytes) || bytes <= 0) return "0 B";
  const units = ["B", "KB", "MB", "GB", "TB"];
  let value = bytes;
  let index = 0;
  while (value >= 1024 && index < units.length - 1) {
    value /= 1024;
    index += 1;
  }
  return `${value >= 10 || index === 0 ? value.toFixed(0) : value.toFixed(1)} ${units[index]}`;
}
