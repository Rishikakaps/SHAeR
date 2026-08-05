#!/usr/bin/env node

import { chromium } from "playwright";
import { spawn, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const OUTPUTS = resolve(HERE, "..");
const OUT = join(HERE, "artifacts", "mac-240x320-qa");
const BASE_URL = process.env.SHAER_THEME_BASE_URL || "http://127.0.0.1:8793";
const PYTHON = process.env.SHAER_PYTHON || process.env.PYTHON || (spawnSync("python3", ["--version"], { stdio: "ignore" }).status === 0 ? "python3" : "python3");
const SETTINGS = ["ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY", "POWER", "DATE_TIME", "SYNC", "ADVANCED"];
const THEMES = [
  "shaer_base_dark",
  "shaer_base_light",
  "shaer_dark_archive",
  "shaer_bombay_ticket",
  "shaer_japanese_punk",
  "shaer_windows_xp",
  "shaer_ghibli_garden",
  "shaer_indian_print"
];

function url(theme, reset = false) {
  return `${BASE_URL}/${theme}/?mode=device&diagnostic=spotify&validation=1&qa=240${reset ? "&reset=1" : ""}`;
}

function row(issue) {
  return {
    "Screen/state": issue.state,
    Theme: issue.theme,
    Action: issue.action,
    Expected: issue.expected,
    Actual: issue.actual,
    Severity: issue.severity
  };
}

async function waitForServer(timeoutMs = 8000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      const response = await fetch(BASE_URL, { redirect: "manual" });
      if (response.status < 500) return;
    } catch {}
    await new Promise((resolvePromise) => setTimeout(resolvePromise, 120));
  }
  throw new Error(`SHAeR server did not become ready at ${BASE_URL}`);
}

function startServer() {
  const parsed = new URL(BASE_URL);
  return spawn(PYTHON, [join(OUTPUTS, "shaer_pi_os", "server.py"), "--host", "127.0.0.1", "--port", parsed.port || "8793"], {
    cwd: OUTPUTS,
    stdio: ["ignore", "pipe", "pipe"],
    env: {
      ...process.env,
      SHAER_CONFIG_DIR: join(OUT, "config"),
      SHAER_RECORDING_TEST_MODE: "1",
      SHAER_RECORDING_MIN_FREE_BYTES: "1"
    }
  });
}

function mdTable(rows) {
  const headers = ["Screen/state", "Theme", "Action", "Expected", "Actual", "Severity"];
  const esc = (value) => String(value ?? "").replace(/\|/g, "\\|").replace(/\n+/g, " ");
  return [
    `| ${headers.join(" | ")} |`,
    `| ${headers.map(() => "---").join(" | ")} |`,
    ...rows.map((entry) => `| ${headers.map((header) => esc(entry[header])).join(" | ")} |`)
  ].join("\n");
}

async function openTheme(page, theme, { reset = false } = {}) {
  await page.goto(url(theme, reset), { waitUntil: "domcontentloaded" });
  await page.waitForFunction(() => window.SHAeRFirmware && window.shaerUI && window.shaerValidation && window.shaerHardware);
}

async function renderCase(page, testCase) {
  await page.evaluate(async ({ state, setting }) => {
    const wait = (ms) => new Promise((resolvePromise) => setTimeout(resolvePromise, ms));
    window.shaerUI.closePopup();
    document.querySelectorAll("[data-shaer-marginalia-overlay]").forEach((node) => node.remove());
    if (state === "settings-subpage") {
      window.SHAeRFirmware.transition("settings", { replace: true, force: true });
      await wait(25);
      window.shaerUI.settingsDomain(setting);
      return;
    }
    if (state === "settings") {
      window.SHAeRFirmware.transition("settings", { replace: true, force: true });
      return;
    }
    if (state === "marginalia") {
      window.shaerValidation.render("now-playing");
      await wait(60);
      const button = document.querySelector("[data-shaer-marginalia]");
      if (button) button.click();
      return;
    }
    if (state === "empty-library") {
      window.dispatchEvent(new CustomEvent("shaer:music-state", {
        detail: {
          status: "ready",
          localTracks: [],
          savedTracks: [],
          playlistTracks: [],
          playlists: [],
          queue: [],
          currentPlayback: null,
          spotify: { configured: false, authenticated: false }
        }
      }));
      window.SHAeRFirmware.transition("library", { replace: true, force: true });
      return;
    }
    if (state === "offline") {
      window.dispatchEvent(new CustomEvent("shaer:music-state", {
        detail: {
          status: "offline",
          error: { message: "SHAeR is offline." },
          localTracks: [],
          savedTracks: [],
          playlists: [],
          queue: [],
          spotify: { configured: true, authenticated: false }
        }
      }));
      window.shaerUI.popup("offline", "NO INTERNET", "SHAeR will reconnect when the network returns.");
      return;
    }
    if (state === "sync") {
      window.SHAeRFirmware.transition("settings", { replace: true, force: true });
      await wait(25);
      window.shaerUI.settingsDomain("SYNC");
      return;
    }
    if (state === "wifi-bluetooth-spotify") {
      window.shaerValidation.render("bluetooth");
      return;
    }
    if (state === "spotify") {
      window.shaerValidation.render("spotify-login");
      return;
    }
    window.shaerValidation.render(state);
  }, testCase);
  await page.waitForTimeout(testCase.state === "boot" || testCase.state === "loading" ? 140 : 80);
}

async function viewportMetrics(page) {
  return page.evaluate(() => {
    const visible = (node) => {
      const style = getComputedStyle(node);
      const rect = node.getBoundingClientRect();
      return style.display !== "none" && style.visibility !== "hidden" && Number(style.opacity) !== 0 && rect.width > 0 && rect.height > 0;
    };
    const rectOf = (node) => {
      const rect = node.getBoundingClientRect();
      return { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom, width: rect.width, height: rect.height };
    };
    const device = document.querySelector(".device");
    const live = document.querySelector("#liveScreen");
    const deviceRect = device ? rectOf(device) : null;
    const liveRect = live ? rectOf(live) : null;
    const nodes = Array.from(document.querySelectorAll(".device *")).filter(visible);
    const escaping = nodes.filter((node) => {
      const rect = rectOf(node);
      return liveRect && (rect.left < liveRect.left - 1 || rect.top < liveRect.top - 1 || rect.right > liveRect.right + 1 || rect.bottom > liveRect.bottom + 1);
    }).map((node) => `${node.tagName.toLowerCase()}${node.className ? "." + String(node.className).split(/\s+/)[0] : ""}`).slice(0, 8);
    const textClips = nodes.filter((node) => {
      const text = node.textContent.trim();
      if (!text || text.length < 2) return false;
      const hasTextChild = Array.from(node.children).some((child) => child.textContent.trim().length > 1);
      const boundText = node.matches("[data-shaer-title], [data-shaer-artist], [data-shaer-album], [data-recording-state], [data-recording-time], [data-recording-storage], .shaer-marginalia-page__meta, .shaer-recording-runtime b, .shaer-recording-runtime strong, .shaer-recording-runtime small");
      if (hasTextChild && !boundText) return false;
      const style = getComputedStyle(node);
      const clips = style.overflow === "hidden" || style.overflowX === "hidden" || style.overflowY === "hidden";
      if (!clips) return false;
      const overflowing = node.scrollWidth > node.clientWidth + 2 || node.scrollHeight > node.clientHeight + 2;
      return overflowing && style.textOverflow !== "ellipsis";
    }).map((node) => node.textContent.trim().slice(0, 48)).slice(0, 8);
    const selected = Array.from(document.querySelectorAll(".device .is-selected, .device [aria-selected='true']")).filter(visible);
    const selectedInside = selected.every((node) => {
      const rect = rectOf(node);
      return liveRect && rect.left >= liveRect.left - 1 && rect.top >= liveRect.top - 1 && rect.right <= liveRect.right + 1 && rect.bottom <= liveRect.bottom + 1;
    });
    const images = Array.from(document.images).filter((image) => image.closest(".device") || image.closest("[data-shaer-marginalia-overlay]"));
    const brokenImages = images.filter((image) => !image.complete || image.naturalWidth === 0).map((image) => image.currentSrc || image.src);
    const fontFamilies = Array.from(new Set(nodes.map((node) => getComputedStyle(node).fontFamily).filter(Boolean))).slice(0, 20);
    return {
      viewport: { width: window.innerWidth, height: window.innerHeight },
      document: { width: document.documentElement.scrollWidth, height: document.documentElement.scrollHeight },
      device: deviceRect,
      live: liveRect,
      escaping,
      textClips,
      selectedCount: selected.length,
      selectedInside,
      brokenImages,
      fontFamilies,
      bodyBackground: getComputedStyle(document.body).backgroundColor,
      deviceBackground: device ? getComputedStyle(device).backgroundColor : ""
    };
  });
}

async function checkCase(page, theme, testCase, issueRows) {
  await renderCase(page, testCase);
  const metrics = await viewportMetrics(page);
  const label = testCase.label || testCase.state;
  const screenshotPath = join(OUT, theme, `${testCase.file || label.toLowerCase().replace(/[^a-z0-9]+/g, "-")}.png`);
  await page.screenshot({ path: screenshotPath, fullPage: false, animations: "disabled" });

  const action = testCase.action || `Render ${label} at 240x320`;
  if (metrics.viewport.width !== 240 || metrics.viewport.height !== 320) {
    issueRows.push(row({ state: label, theme, action, expected: "Browser viewport is exactly 240x320", actual: `${metrics.viewport.width}x${metrics.viewport.height}`, severity: "BLOCKER" }));
  }
  if (!metrics.device || Math.round(metrics.device.width) !== 240 || Math.round(metrics.device.height) !== 320) {
    issueRows.push(row({ state: label, theme, action, expected: "Device frame is exactly 240x320", actual: metrics.device ? `${Math.round(metrics.device.width)}x${Math.round(metrics.device.height)}` : "missing .device", severity: "BLOCKER" }));
  }
  if (metrics.document.width > 242 || metrics.document.height > 322) {
    issueRows.push(row({ state: label, theme, action, expected: "No page scroll outside the device viewport", actual: `document ${metrics.document.width}x${metrics.document.height}`, severity: "MAJOR" }));
  }
  if (metrics.escaping.length) {
    issueRows.push(row({ state: label, theme, action, expected: "Visible layers stay inside the 240x320 screen", actual: metrics.escaping.join(", "), severity: "VISUAL" }));
  }
  if (metrics.textClips.length) {
    issueRows.push(row({ state: label, theme, action, expected: "No clipped essential text", actual: metrics.textClips.join("; "), severity: "VISUAL" }));
  }
  if (metrics.selectedCount && !metrics.selectedInside) {
    issueRows.push(row({ state: label, theme, action, expected: "Selection remains visible", actual: "Selected element is outside the viewport", severity: "MAJOR" }));
  }
  if (metrics.brokenImages.length) {
    issueRows.push(row({ state: label, theme, action, expected: "Artwork loads without internet", actual: metrics.brokenImages.join(", "), severity: "VISUAL" }));
  }
  return { screenshot: screenshotPath, metrics };
}

async function interactionChecks(page, theme, issueRows) {
  await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
  await page.waitForTimeout(60);
  const before = await page.evaluate(() => window.SHAeRFirmware.snapshot());
  for (let index = 0; index < 14; index += 1) await page.keyboard.press("ArrowRight");
  await page.waitForTimeout(40);
  const afterForward = await page.evaluate(() => ({
    snapshot: window.SHAeRFirmware.snapshot(),
    selectedVisible: (() => {
      const node = document.querySelector("#liveScreen .is-selected");
      const live = document.querySelector("#liveScreen").getBoundingClientRect();
      if (!node) return false;
      const rect = node.getBoundingClientRect();
      return rect.top >= live.top - 1 && rect.bottom <= live.bottom + 1;
    })()
  }));
  for (let index = 0; index < 20; index += 1) await page.keyboard.press("ArrowLeft");
  await page.waitForTimeout(40);
  const afterBack = await page.evaluate(() => window.SHAeRFirmware.snapshot());
  if (!(afterForward.snapshot.selectedIndex >= before.selectedIndex && afterForward.snapshot.windowStart > 0 && afterForward.selectedVisible)) {
    issueRows.push(row({ state: "Settings scroll", theme, action: "Rotate encoder through Settings", expected: "List scrolls and selected row remains visible", actual: JSON.stringify(afterForward), severity: "MAJOR" }));
  }
  if (afterBack.selectedIndex !== 0 || afterBack.windowStart !== 0) {
    issueRows.push(row({ state: "Settings scroll", theme, action: "Rotate encoder above first row", expected: "Focus clamps at first Settings row", actual: JSON.stringify(afterBack), severity: "MAJOR" }));
  }

  await page.evaluate(() => window.SHAeRFirmware.transition("library", { replace: true, force: true }));
  await page.waitForTimeout(60);
  const libraryBefore = await page.evaluate(() => window.SHAeRFirmware.snapshot());
  for (let index = 0; index < 12; index += 1) await page.keyboard.press("ArrowRight");
  await page.waitForTimeout(40);
  const libraryAfter = await page.evaluate(() => window.SHAeRFirmware.snapshot());
  if (libraryBefore.visibleRows > 1 && libraryAfter.selectedIndex === libraryBefore.selectedIndex) {
    issueRows.push(row({ state: "My Music scroll", theme, action: "Rotate encoder through My Music", expected: "Focus moves through the library list", actual: JSON.stringify(libraryAfter), severity: "MAJOR" }));
  }

  await page.evaluate(() => {
    window.SHAeRFirmware.transition("home", { replace: true, force: true });
    window.SHAeRFirmware.transition("library", { force: true, skipLoadingInterstitial: true });
    window.SHAeRFirmware.transition("album", { force: true, skipLoadingInterstitial: true });
  });
  await page.waitForTimeout(60);
  const fromAlbum = await page.evaluate(() => window.SHAeRFirmware.snapshot().page);
  await page.keyboard.press("Backspace");
  await page.waitForTimeout(1500);
  const backResult = await page.evaluate(() => window.SHAeRFirmware.snapshot().page);
  if (fromAlbum === "album" && backResult !== "library") {
    issueRows.push(row({ state: "Playlist/Album", theme, action: "Press Back from album", expected: "Returns to My Music/library", actual: backResult, severity: "MAJOR" }));
  }

  await page.evaluate(() => window.SHAeRFirmware.transition("now-playing", { replace: true, force: true }));
  await page.waitForTimeout(80);
  const pageBeforeDoubleOk = await page.evaluate(() => window.SHAeRFirmware.snapshot().page);
  await page.evaluate(async () => {
    window.shaerHardware.test("select");
    await new Promise((resolvePromise) => setTimeout(resolvePromise, 80));
    window.shaerHardware.test("select");
  });
  await page.waitForTimeout(120);
  const doubleOk = await page.evaluate(() => ({ page: window.SHAeRFirmware.snapshot().page, inputMode: window.shaerHardware.inputMode }));
  if (doubleOk.inputMode !== "volume" || doubleOk.page !== pageBeforeDoubleOk) {
    issueRows.push(row({ state: "Double-OK volume", theme, action: "Double press OK on Now Playing", expected: "Enters volume mode without activating a route", actual: JSON.stringify(doubleOk), severity: "MAJOR" }));
  }
  await page.evaluate(() => window.shaerHardware.test("back"));
  await page.waitForTimeout(60);

  await page.keyboard.press("v");
  await page.waitForTimeout(60);
  const vEntered = await page.evaluate(() => window.shaerHardware.inputMode);
  await page.keyboard.press("v");
  await page.waitForTimeout(60);
  const vExited = await page.evaluate(() => window.shaerHardware.inputMode);
  if (vEntered !== "volume" || vExited !== "navigation") {
    issueRows.push(row({ state: "Keyboard shortcut", theme, action: "Press V twice", expected: "V enters then exits volume mode in validation/laptop QA", actual: `after first=${vEntered}, after second=${vExited}`, severity: "MAJOR" }));
  }
}

async function persistenceCheck(page, theme, issueRows) {
  await page.goto(url(theme, true), { waitUntil: "domcontentloaded" });
  await page.waitForFunction(() => window.SHAeRFirmware && window.shaerUI);
  await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
  await page.waitForTimeout(80);
  await page.goto(url(theme, false), { waitUntil: "domcontentloaded" });
  await page.waitForFunction(() => window.SHAeRFirmware);
  await page.waitForTimeout(80);
  const restored = await page.evaluate(() => window.SHAeRFirmware.snapshot().page);
  if (restored !== "settings") {
    issueRows.push(row({ state: "Settings persistence", theme, action: "Reload after opening Settings", expected: "Firmware restores Settings route", actual: restored, severity: "MAJOR" }));
  }
}

async function shortTitleCheck(page, theme, issueRows) {
  await page.evaluate(() => {
    const track = {
      id: "short-title",
      title: "River",
      artist: "Adi",
      artistText: "Adi",
      album: "Home",
      durationMs: 181000,
      uri: "local:short-title",
      source: "local",
      isLocal: true
    };
    window.dispatchEvent(new CustomEvent("shaer:music-state", {
      detail: {
        status: "ready",
        localTracks: [track],
        savedTracks: [track],
        playlistTracks: [track],
        playlists: [{ name: "Short Set", trackCount: 1, uri: "local:playlist:short" }],
        queue: [track],
        currentPlayback: { currentTrack: track, durationMs: 181000, progressMs: 42000, isPlaying: true, source: "local" },
        spotify: { configured: false, authenticated: false }
      }
    }));
    window.SHAeRFirmware.transition("now-playing", { replace: true, force: true });
  });
  await page.waitForTimeout(90);
  const metrics = await viewportMetrics(page);
  await page.screenshot({ path: join(OUT, theme, "short-title.png"), fullPage: false, animations: "disabled" });
  if (metrics.textClips.length) {
    issueRows.push(row({ state: "Short title", theme, action: "Render short Now Playing title", expected: "Short metadata remains unclipped", actual: metrics.textClips.join("; "), severity: "VISUAL" }));
  }
}

async function main() {
  mkdirSync(OUT, { recursive: true });
  for (const theme of THEMES) mkdirSync(join(OUT, theme), { recursive: true });
  const issueRows = [];
  const screenshots = {};
  const externalRequests = [];
  const consoleMessages = [];

  let server = null;
  try {
    try {
      await waitForServer(600);
    } catch {
      server = startServer();
      server.stderr.on("data", (chunk) => process.stderr.write(chunk));
      await waitForServer(60_000);
    }

    const systemChrome = process.env.CHROME_PATH || "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
    const browser = await chromium.launch({ headless: true, ...(existsSync(systemChrome) ? { executablePath: systemChrome } : {}) });
    const context = await browser.newContext({ viewport: { width: 240, height: 320 }, deviceScaleFactor: 1 });
    await context.route("**/*", async (route) => {
      const requestUrl = new URL(route.request().url());
      if (!["127.0.0.1", "localhost"].includes(requestUrl.hostname) && requestUrl.protocol.startsWith("http")) {
        externalRequests.push(route.request().url());
        await route.abort();
        return;
      }
      await route.continue();
    });
    const page = await context.newPage();
    page.on("console", (message) => {
      if (["error", "warning"].includes(message.type())) consoleMessages.push(`${message.type()}: ${message.text()}`);
    });
    page.on("pageerror", (error) => consoleMessages.push(`pageerror: ${error.message}`));

    const baseCases = [
      { state: "boot", label: "Boot" },
      { state: "home", label: "Home" },
      { state: "library", label: "My Music" },
      { state: "album", label: "Playlist/Album" },
      { state: "now-playing", label: "Now Playing" },
      { state: "queue", label: "Queue" },
      { state: "marginalia", label: "Marginalia" },
      { state: "recording", label: "Voice Memos" },
      { state: "settings", label: "Settings" },
      { state: "wifi-bluetooth-spotify", label: "Wi-Fi/Bluetooth" },
      { state: "spotify", label: "Spotify" },
      { state: "sync", label: "Sync" },
      { state: "loading", label: "Loading" },
      { state: "empty-library", label: "Empty" },
      { state: "offline", label: "Offline" },
      { state: "error", label: "Error" }
    ];

    for (const theme of THEMES) {
      screenshots[theme] = [];
      await openTheme(page, theme, { reset: true });
      for (const testCase of baseCases) {
        const result = await checkCase(page, theme, testCase, issueRows);
        screenshots[theme].push(result.screenshot);
      }
      for (const setting of SETTINGS) {
        const result = await checkCase(page, theme, {
          state: "settings-subpage",
          label: `Settings ${setting}`,
          setting,
          file: `settings-${setting.toLowerCase().replace(/_/g, "-")}`,
          action: `Open Settings > ${setting}`
        }, issueRows);
        screenshots[theme].push(result.screenshot);
      }
      await interactionChecks(page, theme, issueRows);
      await persistenceCheck(page, theme, issueRows);
      await shortTitleCheck(page, theme, issueRows);
      if (theme === "shaer_base_light") {
        await page.goto(url(theme, true), { waitUntil: "domcontentloaded" });
        await page.waitForFunction(() => window.SHAeRFirmware);
        await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
        const light = await page.evaluate(() => {
          const device = document.querySelector(".device");
          const style = getComputedStyle(device);
          const bg = style.backgroundColor;
          const match = bg.match(/\d+/g)?.map(Number) || [];
          const brightness = match.length >= 3 ? (match[0] * 299 + match[1] * 587 + match[2] * 114) / 1000 : 0;
          return { bg, brightness };
        });
        if (light.brightness < 170) {
          issueRows.push(row({ state: "Base Light", theme, action: "Render Settings in Base Light", expected: "Light theme remains visibly light", actual: `${light.bg}, brightness ${Math.round(light.brightness)}`, severity: "VISUAL" }));
        }
      }
    }

    await browser.close();

    for (const request of Array.from(new Set(externalRequests))) {
      issueRows.push(row({ state: "Offline assets", theme: "all", action: "Block non-local network requests during QA", expected: "No artwork/font/script requires internet", actual: request, severity: "MAJOR" }));
    }
    for (const message of Array.from(new Set(consoleMessages)).slice(0, 40)) {
      if (/favicon|Failed to load resource: the server responded with a status of 404/.test(message)) continue;
      issueRows.push(row({ state: "Console", theme: "all", action: "Load and walk 240x320 QA states", expected: "No console warnings/errors", actual: message, severity: "CLEANUP" }));
    }
    const report = {
      generatedAt: new Date().toISOString(),
      viewport: "240x320",
      themes: THEMES,
      issueCount: issueRows.length,
      screenshots,
      issues: issueRows
    };
    writeFileSync(join(OUT, "report.json"), JSON.stringify(report, null, 2));
    writeFileSync(join(OUT, "issues.md"), `# SHAeR 240x320 Laptop QA\n\nViewport: 240x320\n\nScreenshots: \`${OUT}\`\n\n${issueRows.length ? mdTable(issueRows) : "No issues recorded."}\n`);
    console.log(JSON.stringify({ viewport: "240x320", themes: THEMES.length, issues: issueRows.length, report: join(OUT, "issues.md") }, null, 2));
    if (issueRows.some((issue) => ["BLOCKER", "MAJOR"].includes(issue.Severity))) process.exitCode = 1;
  } finally {
    if (server) server.kill("SIGTERM");
  }
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
