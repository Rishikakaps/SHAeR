#!/usr/bin/env node

import { chromium } from "playwright";
import { spawn, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const OUTPUTS = resolve(HERE, "..");
const BUNDLED_PYTHON = join(
  process.env.HOME || "",
  ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3"
);
const SYSTEM_PYTHON = spawnSync("python3", ["--version"], { stdio: "ignore" }).status === 0 ? "python3" : "";
const PYTHON = process.env.SHAER_PYTHON || process.env.PYTHON || SYSTEM_PYTHON || (existsSync(BUNDLED_PYTHON) ? BUNDLED_PYTHON : "python3");
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
const STATES = [
  "boot",
  "home",
  "library",
  "album",
  "now-playing",
  "recording",
  "volume",
  "queue",
  "bluetooth",
  "spotify-login",
  "loading",
  "error",
  "shutdown"
];
const OVERLAY_STATES = new Set([
  "boot", "volume", "queue", "bluetooth", "spotify-login", "error", "shutdown"
]);
const THEME_NAVIGATION = {
  shaer_base_dark: { album: true },
  shaer_base_light: { album: true },
  shaer_dark_archive: { album: true },
  shaer_bombay_ticket: { album: true },
  shaer_japanese_punk: { album: true },
  shaer_windows_xp: { album: false },
  shaer_ghibli_garden: { album: false },
  shaer_indian_print: { album: false }
};

function args() {
  const values = process.argv.slice(2);
  const option = (name, fallback) => {
    const index = values.indexOf(name);
    return index >= 0 && values[index + 1] ? values[index + 1] : fallback;
  };
  return {
    baseUrl: option("--base-url", "http://127.0.0.1:8790"),
    updateBaselines: values.includes("--update-baselines"),
    compareBaselines: !values.includes("--no-baseline"),
    keepServer: values.includes("--keep-server"),
    staticMode: values.includes("--static")
  };
}

function themeUrl(config, theme, source = "spotify") {
  const query = `mode=device&diagnostic=${source}&validation=1`;
  if (config.staticMode) return `file://${join(OUTPUTS, theme, "index.html")}?${query}`;
  return `${config.baseUrl}/${theme}/?${query}`;
}

async function waitForServer(url, timeoutMs = 8000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      const response = await fetch(url, { redirect: "manual" });
      if (response.status < 500) return;
    } catch {}
    await new Promise((resolvePromise) => setTimeout(resolvePromise, 120));
  }
  throw new Error(`SHAeR server did not become ready at ${url}`);
}

function startServer(baseUrl) {
  const parsed = new URL(baseUrl);
  if (!["127.0.0.1", "localhost"].includes(parsed.hostname)) return null;
  const child = spawn(
    PYTHON,
    [join(OUTPUTS, "shaer_pi_os", "server.py"), "--host", "127.0.0.1", "--port", parsed.port || "8790"],
    {
      cwd: OUTPUTS,
      stdio: ["ignore", "pipe", "pipe"],
      env: {
        ...process.env,
        SHAER_CONFIG_DIR: join(HERE, "artifacts", "config"),
        SHAER_RECORDING_TEST_MODE: "1",
        SHAER_RECORDING_MIN_FREE_BYTES: "1"
      }
    }
  );
  child.stdout.on("data", () => {});
  child.stderr.on("data", (chunk) => process.stderr.write(chunk));
  return child;
}

async function layoutRules(page, state) {
  return page.evaluate(({ stateName, overlayExpected }) => {
    const errors = [];
    const warnings = [];
    const device = document.querySelector(".device");
    const live = document.querySelector("#liveScreen");
    if (!device || !live) return { errors: ["Missing .device or #liveScreen"], warnings, metrics: {} };
    const rootRect = device.getBoundingClientRect();
    const liveRect = live.getBoundingClientRect();
    const ratio = liveRect.width / liveRect.height;
    if (Math.abs(ratio - 0.75) > 0.04) errors.push(`Device ratio ${ratio.toFixed(3)} is not 3:4`);
    if (liveRect.width < 200 || liveRect.height < 260) errors.push("Live screen rendered below minimum validation dimensions");

    const visible = (node) => {
      const style = getComputedStyle(node);
      const rect = node.getBoundingClientRect();
      return style.display !== "none" && style.visibility !== "hidden" && Number(style.opacity) !== 0 && rect.width > 0 && rect.height > 0;
    };
    const contained = (outer, inner, tolerance = 1.5) => (
      inner.left >= outer.left - tolerance && inner.top >= outer.top - tolerance &&
      inner.right <= outer.right + tolerance && inner.bottom <= outer.bottom + tolerance
    );
    const liveCandidates = overlayExpected ? "" : (
      "#liveScreen button, #liveScreen [data-shaer-title], #liveScreen [data-shaer-artist], " +
      "#liveScreen [data-shaer-album], #liveScreen [data-shaer-progress], #liveScreen [data-shaer-cover], " +
      "#liveScreen .shaer-recording-runtime, "
    );
    const candidates = Array.from(document.querySelectorAll(
      liveCandidates + "#shaer-system-layer .shaer-system-popup, #shaer-system-layer button"
    )).filter(visible);
    for (const node of candidates) {
      const rect = node.getBoundingClientRect();
      const outer = node.closest("#shaer-system-layer") ? rootRect : liveRect;
      if (!contained(outer, rect)) {
        errors.push(`${node.tagName.toLowerCase()}${node.className ? "." + String(node.className).split(" ")[0] : ""} escapes screen bounds`);
      }
    }

    const textNodes = Array.from(document.querySelectorAll(
      "[data-shaer-title], [data-shaer-artist], [data-shaer-album], .shaer-system-popup h2, .shaer-system-popup p, .shaer-system-actions button"
    )).filter(visible);
    for (const node of textNodes) {
      const allowsMotion = node.classList.contains("shaer-long-text");
      const ellipsized = getComputedStyle(node).textOverflow === "ellipsis";
      if (!allowsMotion && !ellipsized && node.scrollWidth > node.clientWidth + 2) errors.push(`Text clips horizontally: ${node.textContent.trim().slice(0, 36)}`);
      if (!ellipsized && node.scrollHeight > node.clientHeight + 2) errors.push(`Text clips vertically: ${node.textContent.trim().slice(0, 36)}`);
    }

    const artwork = Array.from(document.querySelectorAll("[data-shaer-cover]")).filter(visible);
    for (const node of artwork) {
      if (!contained(liveRect, node.getBoundingClientRect())) errors.push("Artwork escapes the live screen");
    }

    const buttons = Array.from(document.querySelectorAll("#shaer-system-layer button")).filter(visible);
    for (let left = 0; left < buttons.length; left += 1) {
      for (let right = left + 1; right < buttons.length; right += 1) {
        const a = buttons[left].getBoundingClientRect();
        const b = buttons[right].getBoundingClientRect();
        const overlap = Math.max(0, Math.min(a.right, b.right) - Math.max(a.left, b.left)) * Math.max(0, Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top));
        if (overlap > 1) errors.push("Popup controls overlap");
      }
    }

    const layer = document.querySelector("#shaer-system-layer");
    const overlayVisible = Boolean(layer && !layer.hidden && visible(layer));
    if (overlayExpected && !overlayVisible) errors.push(`Expected ${stateName} overlay is not visible`);
    if (!overlayExpected && overlayVisible) errors.push(`Unexpected overlay remains visible on ${stateName}`);
    if (stateName === "now-playing") {
      if (!document.querySelector("[data-shaer-title]")) errors.push("Now Playing has no title binding");
      if (!document.querySelector("[data-shaer-progress]")) errors.push("Now Playing has no progress binding");
    }
    if (stateName === "recording") {
      if (!document.querySelector(".shaer-recording-runtime")) errors.push("Recording state has no runtime status");
      if (!document.querySelector('[data-action="toggle-memo"]')) errors.push("Recording state has no hardware-select action");
    }
    if (stateName === "queue" && document.querySelectorAll(".shaer-system-queue div").length < 2) errors.push("Queue does not render fixture rows");

    return {
      errors: Array.from(new Set(errors)),
      warnings: Array.from(new Set(warnings)),
      metrics: {
        width: Math.round(liveRect.width * 10) / 10,
        height: Math.round(liveRect.height * 10) / 10,
        ratio: Math.round(ratio * 1000) / 1000,
        boundElements: candidates.length,
        artworkElements: artwork.length,
        overlayVisible
      }
    };
  }, { stateName: state, overlayExpected: OVERLAY_STATES.has(state) });
}

async function providerGeometry(page, config, theme, source) {
  await page.goto(themeUrl(config, theme, source), { waitUntil: "domcontentloaded" });
  await page.waitForFunction(() => window.shaerValidation && window.shaerUI);
  await page.evaluate(() => window.shaerValidation.render("now-playing"));
  await page.waitForTimeout(60);
  return page.evaluate(() => {
    const select = (selector) => {
      const node = document.querySelector(selector);
      if (!node) return null;
      const rect = node.getBoundingClientRect();
      return { x: Math.round(rect.x), y: Math.round(rect.y), width: Math.round(rect.width), height: Math.round(rect.height) };
    };
    return {
      title: select("[data-shaer-title]"),
      artist: select("[data-shaer-artist]"),
      progress: select("[data-shaer-progress]"),
      artwork: select("[data-shaer-cover]")
    };
  });
}

async function firmwareContract(page, theme) {
  return page.evaluate(async ({ expectedAlbum }) => {
    const errors = [];
    const firmware = window.SHAeRFirmware;
    if (!firmware) return { errors: ["Shared firmware core is not mounted"] };

    const expectPage = (expected, label) => {
      const actual = firmware.snapshot().page;
      if (actual !== expected) errors.push(`${label}: expected ${expected}, got ${actual}`);
    };
    const selectionErrors = () => {
      const visible = Array.from(document.querySelectorAll("#liveScreen [data-nav]:not([hidden]):not([disabled])"));
      const selected = visible.filter((node) => node.classList.contains("is-selected"));
      if (visible.length && selected.length !== 1) errors.push(`Expected one encoder selection, found ${selected.length}`);
    };

    firmware.transition("home", { replace: true, force: true });
    expectPage("home", "home transition");
    selectionErrors();
    firmware.transition("library", { force: true, skipLoadingInterstitial: true });
    expectPage("library", "library transition");
    selectionErrors();
    firmware.transition("album", { force: true, skipLoadingInterstitial: true });
    expectPage(expectedAlbum ? "album" : "library", "album capability route");
    firmware.transition("now-playing", { force: true, skipLoadingInterstitial: true });
    expectPage("now-playing", "now-playing transition");
    firmware.back();
    await new Promise((resolvePromise) => setTimeout(resolvePromise, 1350));
    expectPage(expectedAlbum ? "album" : "library", "back from now-playing");

    window.dispatchEvent(new CustomEvent("shaer:playback", {
      detail: { source: "spotify", status: "playing", title: "", uri: "" }
    }));
    if (firmware.snapshot().playback.playing) errors.push("Impossible playing-without-track state was accepted");

    firmware.dispatch("play");
    firmware.dispatch("toggle-memo");
    const exclusive = firmware.snapshot();
    if (exclusive.playback.playing && exclusive.recording) errors.push("Playback and recording are active simultaneously");
    if (!firmware.logs().some((entry) => entry.event === "rejected-impossible-playback")) {
      errors.push("Impossible provider state was not logged");
    }
    firmware.dispatch("toggle-memo");

    firmware.transition("settings", { replace: true, force: true });
    await new Promise((resolvePromise) => setTimeout(resolvePromise, 25));
    const visibleSettings = Array.from(document.querySelectorAll("#liveScreen [data-setting]"))
      .filter((node) => {
        const style = getComputedStyle(node);
        return !node.hidden && style.display !== "none" && style.visibility !== "hidden";
      })
      .map((node) => String(node.dataset.setting || "").trim().toUpperCase());
    const allowed = new Set([
      "ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY",
      "POWER", "DATE_TIME", "DATE & TIME", "SYNC", "ADVANCED"
    ]);
    const unsupported = visibleSettings.filter((setting) => !allowed.has(setting));
    if (unsupported.length) errors.push(`Visible settings lack firmware behavior: ${unsupported.join(", ")}`);
    if (visibleSettings.includes("RECORDER")) errors.push("Recorder must not be a top-level Settings category");
    if (!visibleSettings.includes("ABOUT")) errors.push("About setting is missing");
    selectionErrors();

    return {
      errors: Array.from(new Set(errors)),
      version: firmware.version,
      navigation: firmware.snapshot().navigation,
      capabilities: firmware.snapshot().capabilities,
      visibleSettings
    };
  }, { expectedAlbum: THEME_NAVIGATION[theme].album });
}

async function main() {
  const config = args();
  const artifacts = join(HERE, "artifacts");
  const baselines = join(HERE, "baselines");
  mkdirSync(artifacts, { recursive: true });
  mkdirSync(baselines, { recursive: true });

  let server = null;
  let browser = null;
  try {
    if (!config.staticMode) {
      try {
        await waitForServer(config.baseUrl, 600);
      } catch {
        server = startServer(config.baseUrl);
        await waitForServer(config.baseUrl, 60_000);
      }
    }

    const systemChrome = process.env.CHROME_PATH || "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
    browser = await chromium.launch({
      headless: true,
      ...(existsSync(systemChrome) ? { executablePath: systemChrome } : {})
    });
    const page = await browser.newPage({ viewport: { width: 480, height: 640 }, deviceScaleFactor: 1 });
    await page.emulateMedia({ reducedMotion: "reduce" });
    const consoleErrors = [];
    page.on("console", (message) => {
      if (["error", "warning"].includes(message.type())) consoleErrors.push(message.text());
    });
    page.on("pageerror", (error) => consoleErrors.push(error.message));

    const report = {
      generatedAt: new Date().toISOString(),
      baseUrl: config.baseUrl,
      themes: {},
      summary: { states: 0, errors: 0, warnings: 0, baselineMismatches: 0 },
      consoleErrors
    };

    for (const theme of THEMES) {
      const themeDir = join(artifacts, theme);
      const baselineDir = join(baselines, theme);
      mkdirSync(themeDir, { recursive: true });
      mkdirSync(baselineDir, { recursive: true });
      await page.goto(themeUrl(config, theme), { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.shaerValidation && window.shaerUI && window.SHAeRFirmware);
      const themeReport = { states: {}, providerParity: null, firmwareContract: null };

      const contract = await firmwareContract(page, theme);
      themeReport.firmwareContract = contract;
      report.summary.errors += contract.errors.length;

      for (const state of STATES) {
        await page.evaluate((stateName) => window.shaerValidation.render(stateName), state);
        await page.waitForTimeout(state === "boot" ? 90 : 55);
        const rules = await layoutRules(page, state);
        const device = page.locator(".device");
        const screenshot = await device.screenshot({ animations: "disabled" });
        const artifactPath = join(themeDir, `${state}.png`);
        const baselinePath = join(baselineDir, `${state}.png`);
        writeFileSync(artifactPath, screenshot);
        let baseline = "missing";
        if (config.updateBaselines) {
          writeFileSync(baselinePath, screenshot);
          baseline = "updated";
        } else if (config.compareBaselines && existsSync(baselinePath)) {
          baseline = "pending-visual-diff";
        }
        themeReport.states[state] = { ...rules, screenshot: artifactPath, baseline };
        report.summary.states += 1;
        report.summary.errors += rules.errors.length;
        report.summary.warnings += rules.warnings.length;
      }

      const localGeometry = await providerGeometry(page, config, theme, "local");
      const spotifyGeometry = await providerGeometry(page, config, theme, "spotify");
      const parity = JSON.stringify(localGeometry) === JSON.stringify(spotifyGeometry);
      themeReport.providerParity = { pass: parity, local: localGeometry, spotify: spotifyGeometry };
      if (!parity) report.summary.errors += 1;
      report.themes[theme] = themeReport;
    }

    if (!config.updateBaselines && config.compareBaselines) {
      const comparison = spawnSync(
        PYTHON,
        [join(HERE, "compare-images.py"), baselines, artifacts],
        { encoding: "utf-8" }
      );
      if (comparison.status !== 0) throw new Error(comparison.stderr || "Visual comparison failed");
      const visual = JSON.parse(comparison.stdout);
      report.visualComparison = visual;
      report.summary.baselineMismatches = visual.mismatches;
      for (const [relative, result] of Object.entries(visual.files)) {
        const [theme, filename] = relative.split("/");
        const state = filename.replace(/\.png$/, "");
        if (report.themes[theme] && report.themes[theme].states[state]) {
          report.themes[theme].states[state].baseline = result.match ? "match" : "mismatch";
          report.themes[theme].states[state].differenceRatio = result.difference_ratio;
        }
      }
    }
    await browser.close();
    browser = null;
    writeFileSync(join(artifacts, "report.json"), JSON.stringify(report, null, 2));
    const failed = report.summary.errors || report.summary.baselineMismatches || consoleErrors.length;
    console.log(JSON.stringify(report.summary));
    console.log(`Report: ${join(artifacts, "report.json")}`);
    if (failed) process.exitCode = 1;
  } finally {
    if (browser) await browser.close();
    if (server && !config.keepServer) server.kill("SIGTERM");
  }
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
