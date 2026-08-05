#!/usr/bin/env node

import { chromium } from "playwright";
import { spawn, spawnSync } from "node:child_process";
import { existsSync, mkdirSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const OUTPUTS = resolve(HERE, "..");
const OUT = join(HERE, "artifacts", "ui-correction");
const BASE_URL = process.env.SHAER_THEME_BASE_URL || "http://127.0.0.1:8790";
const PYTHON = process.env.SHAER_PYTHON || process.env.PYTHON || (spawnSync("python3", ["--version"], { stdio: "ignore" }).status === 0 ? "python3" : "python3");

const screenshots = [
  ["base-dark-settings-scroll", "shaer_base_dark", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    for (let index = 0; index < 6; index += 1) await page.keyboard.press("ArrowRight");
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "base-dark-settings-scroll.png"), animations: "disabled" });
  }],
  ["base-light-playback-subpage", "shaer_base_light", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("Enter");
    await page.locator(".device").screenshot({ path: join(OUT, "base-light-playback-subpage.png"), animations: "disabled" });
  }],
  ["bombay-home", "shaer_bombay_ticket", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("home", { replace: true, force: true }));
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "bombay-home.png"), animations: "disabled" });
  }],
  ["bombay-voice-memo", "shaer_bombay_ticket", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("recordings", { replace: true, force: true }));
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "bombay-voice-memo.png"), animations: "disabled" });
  }],
  ["japanese-punk-non-about-focused", "shaer_japanese_punk", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("ArrowRight");
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "japanese-punk-non-about-focused.png"), animations: "disabled" });
  }],
  ["windows-xp-settings", "shaer_windows_xp", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "windows-xp-settings.png"), animations: "disabled" });
  }],
  ["ghibli-home", "shaer_ghibli_garden", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("home", { replace: true, force: true }));
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "ghibli-home.png"), animations: "disabled" });
  }],
  ["ghibli-settings-scroll", "shaer_ghibli_garden", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    for (let index = 0; index < 5; index += 1) await page.keyboard.press("ArrowRight");
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "ghibli-settings-scroll.png"), animations: "disabled" });
  }],
  ["indian-raga-home", "shaer_indian_print", async (page) => {
    await page.evaluate(() => window.SHAeRFirmware.transition("home", { replace: true, force: true }));
    await page.locator("#liveScreen").screenshot({ path: join(OUT, "indian-raga-home.png"), animations: "disabled" });
  }]
];

function url(theme) {
  return `${BASE_URL}/${theme}/?mode=device&diagnostic=spotify&validation=1&reset=1`;
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
  return spawn(PYTHON, [join(OUTPUTS, "shaer_pi_os", "server.py"), "--host", "127.0.0.1", "--port", parsed.port || "8790"], {
    cwd: OUTPUTS,
    stdio: ["ignore", "pipe", "pipe"],
    env: { ...process.env, SHAER_CONFIG_DIR: join(HERE, "artifacts", "config"), SHAER_RECORDING_TEST_MODE: "1", SHAER_RECORDING_MIN_FREE_BYTES: "1" }
  });
}

async function openTheme(page, theme) {
  await page.goto(url(theme), { waitUntil: "domcontentloaded" });
  await page.waitForFunction(() => window.SHAeRFirmware && window.shaerUI);
}

async function main() {
  mkdirSync(OUT, { recursive: true });
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
    const page = await browser.newPage({ viewport: { width: 480, height: 640 }, deviceScaleFactor: 1 });
    await page.emulateMedia({ reducedMotion: "reduce" });
    const errors = [];

    await openTheme(page, "shaer_base_dark");
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    const before = await page.evaluate(() => ({ ...window.SHAeRFirmware.snapshot(), thumb: getComputedStyle(document.querySelector(".position-indicator span")).marginTop }));
    for (let index = 0; index < 6; index += 1) await page.keyboard.press("ArrowRight");
    const after = await page.evaluate(() => ({ ...window.SHAeRFirmware.snapshot(), thumb: getComputedStyle(document.querySelector(".position-indicator span")).marginTop }));
    for (let index = 0; index < 12; index += 1) await page.keyboard.press("ArrowLeft");
    const clamped = await page.evaluate(() => window.SHAeRFirmware.snapshot());
    if (!(after.selectedIndex === 6 && after.windowStart > before.windowStart)) errors.push("Base Dark selection did not advance the scroll window.");
    if (after.thumb === before.thumb) errors.push("Scrollbar thumb did not move with scroll window.");
    if (clamped.selectedIndex !== 0 || clamped.windowStart !== 0) errors.push("Selection did not clamp at the first item.");

    await openTheme(page, "shaer_base_light");
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("Enter");
    const lightTokens = await page.$eval("#shaer-system-layer", (node) => {
      const style = getComputedStyle(node);
      return { panel: style.getPropertyValue("--shaer-panel").trim(), fg: style.getPropertyValue("--shaer-fg").trim() };
    });
    if (!lightTokens.panel.includes("#ffffff") || !lightTokens.fg.includes("#141414")) errors.push("Base Light Settings overlay does not resolve to light tokens.");

    await openTheme(page, "shaer_bombay_ticket");
    const bombay = await page.evaluate(() => {
      window.SHAeRFirmware.transition("home", { replace: true, force: true });
      const live = document.querySelector("#liveScreen").getBoundingClientRect();
      const lines = Array.from(document.querySelectorAll(".menu-line, .menu-vline")).map((node) => {
        const rect = node.getBoundingClientRect();
        return rect.left >= live.left && rect.top >= live.top && rect.right <= live.right && rect.bottom <= live.bottom;
      });
      const text = document.querySelector("#liveScreen").innerText;
      return {
        linesInside: lines.every(Boolean),
        hasChaiRoute: Boolean(document.querySelector(".home-stamp,[data-target='chai-ticket']")),
        hasArchive: text.includes("ARCHIVE"),
        hasFakeStorage: /7\\.1\\s*GB|GB\\s*FREE/.test(text)
      };
    });
    if (!bombay.linesInside) errors.push("Bombay ticket rules escape the viewport.");
    if (bombay.hasChaiRoute) errors.push("Bombay SHAeR branding can route to chai/charging.");
    if (bombay.hasArchive || bombay.hasFakeStorage) errors.push("Bombay Voice Memo has an unintended Archive/storage overlay.");

    await openTheme(page, "shaer_japanese_punk");
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    await page.keyboard.press("ArrowRight");
    await page.keyboard.press("ArrowRight");
    const punkHighlights = await page.$$eval(".setting-row.active, .setting-row.is-selected", (nodes) => nodes.length);
    if (punkHighlights !== 1) errors.push(`Japanese Punk highlighted row count is ${punkHighlights}, expected 1.`);

    await openTheme(page, "shaer_windows_xp");
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    const xpOverlap = await page.evaluate(() => {
      const menu = document.querySelector(".settings-menu").getBoundingClientRect();
      const taskbar = document.querySelector(".taskbar").getBoundingClientRect();
      return menu.bottom > taskbar.top;
    });
    if (xpOverlap) errors.push("Windows XP Settings window overlaps the taskbar.");

    await openTheme(page, "shaer_ghibli_garden");
    await page.evaluate(() => window.SHAeRFirmware.transition("settings", { replace: true, force: true }));
    const ghibli = await page.evaluate(() => {
      const device = document.querySelector(".device").getBoundingClientRect();
      const row = document.querySelector(".setting-row");
      return {
        device: { width: Math.round(device.width), height: Math.round(device.height) },
        fontSize: Number.parseFloat(getComputedStyle(row).fontSize),
        background: getComputedStyle(document.querySelector(".garden-bg")).backgroundImage
      };
    });
    if (ghibli.device.width !== 240 || ghibli.device.height !== 320) errors.push("Ghibli device mode leaves outer gutters.");
    if (ghibli.fontSize < 9.5) errors.push("Ghibli Settings text is too small.");
    if (!ghibli.background.includes("forest-settings.png")) errors.push("Ghibli Settings artwork is not loaded.");

    await openTheme(page, "shaer_indian_print");
    const ragaAssets = await page.evaluate(async () => {
      const required = [
        "assets/screens/blue-elephants.png",
        "assets/screens/red-hands.png",
        "assets/screens/pink-floral.png",
        "assets/screens/teal-floral.png",
        "assets/screens/black-leaves.png",
        "assets/borders/blue-home.svg",
        "assets/borders/black-settings.svg"
      ];
      const results = {};
      await Promise.all(required.map((src) => new Promise((resolve) => {
        const image = new Image();
        image.onload = () => { results[src] = true; resolve(); };
        image.onerror = () => { results[src] = false; resolve(); };
        image.src = src;
      })));
      return results;
    });
    Object.entries(ragaAssets).forEach(([asset, ok]) => { if (!ok) errors.push(`Indian Raga missing approved bitmap asset: ${asset}`); });

    for (const [, theme, action] of screenshots) {
      await openTheme(page, theme);
      await action(page);
    }

    await browser.close();
    console.log(JSON.stringify({ errors: errors.length, screenshots: OUT }, null, 2));
    if (errors.length) {
      for (const error of errors) console.error(error);
      process.exitCode = 1;
    }
  } finally {
    if (server) server.kill("SIGTERM");
  }
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
