#!/usr/bin/env node

import { chromium } from "playwright";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { spawn, spawnSync } from "node:child_process";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const OUTPUTS = resolve(HERE, "..");
const ARTIFACTS = join(HERE, "artifacts");
const CELLS = join(ARTIFACTS, "contact-sheet-cells");
const OUT = join(ARTIFACTS, "theme-contact-sheet.png");
const HTML = join(ARTIFACTS, "theme-contact-sheet.html");
const BUNDLED_PYTHON = join(
  process.env.HOME || "",
  ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3"
);
const SYSTEM_PYTHON = spawnSync("python3", ["--version"], { stdio: "ignore" }).status === 0 ? "python3" : "";
const PYTHON = process.env.SHAER_PYTHON || process.env.PYTHON || SYSTEM_PYTHON || (existsSync(BUNDLED_PYTHON) ? BUNDLED_PYTHON : "python3");
const BASE_URL = process.env.SHAER_THEME_BASE_URL || "http://127.0.0.1:8790";
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
  return `${BASE_URL}/${theme}/?mode=device&diagnostic=spotify&validation=1`;
}

function img(theme, state) {
  const file = join(CELLS, theme, `${state}.png`);
  if (!existsSync(file)) throw new Error(`Missing contact-sheet cell: ${file}`);
  return `<figure><img src="${pathToFileURL(file).href}" width="240" height="320" alt="${theme} ${state}"><figcaption>${theme}<br>${state}</figcaption></figure>`;
}

async function main() {
  mkdirSync(ARTIFACTS, { recursive: true });
  mkdirSync(CELLS, { recursive: true });
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
    await page.emulateMedia({ reducedMotion: "reduce" });
    for (const theme of THEMES) {
      const themeDir = join(CELLS, theme);
      mkdirSync(themeDir, { recursive: true });
      await page.goto(themeUrl(theme), { waitUntil: "domcontentloaded" });
      await page.waitForFunction(() => window.shaerValidation && window.shaerUI);
      for (const state of STATES) {
        await page.evaluate((stateName) => window.shaerValidation.render(stateName), state);
        await page.waitForTimeout(state === "boot" ? 90 : 55);
        await page.locator("#liveScreen").screenshot({ path: join(themeDir, `${state}.png`), animations: "disabled" });
      }
    }

    await browser.close();
  } finally {
    if (server) server.kill("SIGTERM");
  }

  const body = STATES.map((state) => `<section><h2>${state}</h2><div class="row">${THEMES.map((theme) => img(theme, state)).join("")}</div></section>`).join("");
  writeFileSync(HTML, `<!doctype html>
<meta charset="utf-8">
<title>SHAeR Theme Contact Sheet</title>
<style>
  * { box-sizing: border-box; }
  body { margin: 0; padding: 16px; background: #f4f4f0; color: #111; font: 12px ui-monospace, SFMono-Regular, Menlo, monospace; }
  h1 { margin: 0 0 14px; font-size: 18px; }
  h2 { margin: 18px 0 8px; font-size: 14px; text-transform: uppercase; }
  .row { display: grid; grid-template-columns: repeat(${THEMES.length}, 240px); gap: 12px; align-items: start; }
  figure { margin: 0; width: 240px; }
  img { display: block; width: 240px; height: 320px; border: 1px solid #111; object-fit: contain; background: #000; }
  figcaption { min-height: 32px; padding-top: 4px; line-height: 1.2; }
</style>
<h1>SHAeR Theme Contact Sheet - 240 x 320 States</h1>
${body}`);

  const systemChrome = process.env.CHROME_PATH || "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
  const browser = await chromium.launch({
    headless: true,
    ...(existsSync(systemChrome) ? { executablePath: systemChrome } : {})
  });
  const page = await browser.newPage({ viewport: { width: 2048, height: 1200 }, deviceScaleFactor: 1 });
  await page.goto(pathToFileURL(HTML).href, { waitUntil: "load" });
  await page.screenshot({ path: OUT, fullPage: true });
  await browser.close();
  console.log(OUT);
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
