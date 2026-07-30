import { describe, expect, it } from "vitest";
import { assertSupportedAudio, formatBytes, normalizeBaseUrl, sanitizeFilename } from "./validation";

describe("desktop validation helpers", () => {
  it("normalizes manual SHAeR addresses without accepting unsupported schemes", () => {
    expect(normalizeBaseUrl("shaer.local:8775/")).toBe("http://shaer.local:8775");
    expect(normalizeBaseUrl("https://shaer.local:8775/path/")).toBe("https://shaer.local:8775/path");
    expect(() => normalizeBaseUrl("file:///tmp/shaer")).toThrow(/HTTP or HTTPS/);
  });

  it("sanitizes filenames before upload", () => {
    expect(sanitizeFilename("../bad:name?.mp3")).toBe(".._bad_name_.mp3");
    expect(sanitizeFilename("\0")).toBe("shaer-file");
  });

  it("gates the first transfer slice to reported supported formats", () => {
    expect(() => assertSupportedAudio("song.mp3", ["mp3"])).not.toThrow();
    expect(() => assertSupportedAudio("song.flac", ["mp3"])).toThrow(/does not report support/);
  });

  it("formats storage values for the dashboard", () => {
    expect(formatBytes(0)).toBe("0 B");
    expect(formatBytes(1024)).toBe("1.0 KB");
    expect(formatBytes(10485760)).toBe("10 MB");
  });
});
