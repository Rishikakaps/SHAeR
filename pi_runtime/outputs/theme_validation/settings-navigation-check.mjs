#!/usr/bin/env node

import { chromium } from "playwright";
import { spawn, spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
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
const SETTINGS = ["ABOUT", "APPEARANCE", "PLAYBACK", "AUDIO", "CONNECTIVITY", "POWER", "DATE_TIME", "SYNC", "ADVANCED"];

function option(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 && process.argv[index + 1] ? process.argv[index + 1] : fallback;
}

const BASE_URL = option("--base-url", "http://127.0.0.1:8790");

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

function startServer() {
  const parsed = new URL(BASE_URL);
  return spawn(PYTHON, [join(OUTPUTS, "shaer_pi_os", "server.py"), "--host", "127.0.0.1", "--port", parsed.port || "8790"], {
    cwd: OUTPUTS,
    stdio: ["ignore", "pipe", "pipe"],
    env: {
      ...process.env,
      SHAER_CONFIG_DIR: join(HERE, "artifacts", "config"),
      SHAER_RECORDING_TEST_MODE: "1",
      SHAER_RECORDING_MIN_FREE_BYTES: "1"
    }
  });
}

function themeUrl(theme) {
  return `${BASE_URL}/${theme}/?mode=device&diagnostic=spotify&validation=1&cb=20260729b`;
}

function normalizeSetting(value) {
  return String(value || "").trim().toUpperCase().replace(/\s*&\s*/g, "_").replace(/\s+/g, "_");
}

async function main() {
  let server = null;
  try {
    try {
      await waitForServer(BASE_URL, 600);
    } catch {
      server = startServer();
      server.stderr.on("data", (chunk) => process.stderr.write(chunk));
      await waitForServer(BASE_URL, 60_000);
    }

    const systemChrome = process.env.CHROME_PATH || "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
    const browser = await chromium.launch({
      headless: true,
      ...(existsSync(systemChrome) ? { executablePath: systemChrome } : {})
    });
    const page = await browser.newPage({ viewport: { width: 480, height: 640 }, deviceScaleFactor: 1 });
    const errors = [];
    const report = {};

    for (const theme of THEMES) {
      await page.goto(themeUrl(theme), { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.shaerValidation && window.SHAeRFirmware && window.shaerUI);
      report[theme] = {};
      for (const [index, setting] of SETTINGS.entries()) {
        await page.evaluate(() => {
          document.querySelector("#shaer-system-layer").hidden = true;
          window.SHAeRFirmware.transition("settings", { replace: true, force: true });
        });
        await page.waitForTimeout(30);
        for (let step = 0; step < index; step += 1) await page.keyboard.press("ArrowRight");
        await page.waitForTimeout(20);
        const selected = await page.evaluate(() => {
          const node = document.querySelector("#liveScreen [data-nav].is-selected");
          return node ? String(node.dataset.setting || "").trim().toUpperCase().replace(/\s*&\s*/g, "_").replace(/\s+/g, "_") : "";
        });
        if (selected !== setting) errors.push(`${theme}: expected selected ${setting}, got ${selected || "none"}`);
        await page.keyboard.press("Enter");
        await page.waitForTimeout(20);
        const routed = await page.$eval("#shaer-system-layer [data-settings-domain]", (node) => node.dataset.settingsDomain);
        report[theme][setting] = routed;
        if (routed !== setting) errors.push(`${theme}: ${setting} routed to ${routed}`);
      }
    }

    await browser.close();
    console.log(JSON.stringify({ themes: THEMES.length, settings: SETTINGS.length, errors: errors.length, report }, null, 2));
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
