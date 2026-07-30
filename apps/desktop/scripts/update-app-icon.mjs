#!/usr/bin/env node
import { copyFileSync, existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { spawnSync } from "node:child_process";

const input = process.argv[2];
if (!input) {
  console.error("Usage: npm run icon:update -- /absolute/path/to/icon.png");
  process.exit(1);
}

const root = resolve("../..");
const source = resolve(input);
if (!existsSync(source)) {
  console.error(`Icon source does not exist: ${source}`);
  process.exit(1);
}

function ensure(path) {
  mkdirSync(dirname(path), { recursive: true });
}

function resize(target, size) {
  ensure(target);
  const result = spawnSync("sips", ["-z", String(size), String(size), source, "--out", target], { stdio: "inherit" });
  if (result.status !== 0) {
    console.error("sips failed. On macOS, install Command Line Tools or provide a PNG already sized for the target.");
    process.exit(result.status || 1);
  }
}

function run(command, args) {
  const result = spawnSync(command, args, { stdio: "inherit" });
  if (result.status !== 0) {
    console.error(`${command} failed while updating app icons.`);
    process.exit(result.status || 1);
  }
}

function copy(target) {
  ensure(target);
  copyFileSync(source, target);
}

function updateDesktopBundleIcons() {
  run("npx", ["tauri", "icon", source]);
}

const targets = [
  ["apps/desktop/src-tauri/icons/icon.png", 512],
  ["apps/desktop/src-tauri/icons/32x32.png", 32],
  ["apps/desktop/src-tauri/icons/128x128.png", 128],
  ["apps/desktop/src-tauri/icons/128x128@2x.png", 256],
  ["pi_runtime/outputs/shaer_companion/icons/shaer-192.png", 192],
  ["pi_runtime/outputs/shaer_companion/icons/shaer-512.png", 512],
  ["pi_runtime/outputs/shaer_companion/dist/icons/shaer-192.png", 192],
  ["pi_runtime/outputs/shaer_companion/dist/icons/shaer-512.png", 512],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/assets/public/icons/shaer-192.png", 192],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/assets/public/icons/shaer-512.png", 512],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-mdpi/ic_launcher.png", 48],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-hdpi/ic_launcher.png", 72],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xhdpi/ic_launcher.png", 96],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxhdpi/ic_launcher.png", 144],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png", 192],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-mdpi/ic_launcher_round.png", 48],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-hdpi/ic_launcher_round.png", 72],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xhdpi/ic_launcher_round.png", 96],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxhdpi/ic_launcher_round.png", 144],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher_round.png", 192],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-mdpi/ic_launcher_foreground.png", 108],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-hdpi/ic_launcher_foreground.png", 162],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xhdpi/ic_launcher_foreground.png", 216],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxhdpi/ic_launcher_foreground.png", 324],
  ["pi_runtime/outputs/shaer_companion/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher_foreground.png", 432]
];

for (const [relative, size] of targets) {
  resize(resolve(root, relative), size);
}

updateDesktopBundleIcons();

copy(resolve(root, "assets/branding/app-icon-source.png"));
writeFileSync(resolve(root, "assets/branding/app-icon.json"), JSON.stringify({
  updatedAt: new Date().toISOString(),
  source: "assets/branding/app-icon-source.png",
  generatedTargets: [
    ...targets.map(([target]) => target),
    "apps/desktop/src-tauri/icons/icon.icns",
    "apps/desktop/src-tauri/icons/icon.ico"
  ]
}, null, 2) + "\n");

console.log(`Updated ${targets.length + 2} SHAeR app icon targets from ${source}`);
