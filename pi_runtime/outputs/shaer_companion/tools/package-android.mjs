import { createHash } from "node:crypto";
import { cp, mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const android = resolve(root, "android");
const home = process.env.HOME || "";
const environment = {
  ...process.env,
  JAVA_HOME: process.env.JAVA_HOME || "/opt/homebrew/opt/openjdk@21",
  ANDROID_HOME: process.env.ANDROID_HOME || resolve(home, "Library", "Android", "sdk")
};
const gradle = process.platform === "win32" ? "gradlew.bat" : "./gradlew";
await new Promise((resolvePromise, reject) => {
  const child = spawn(gradle, ["assembleDebug", "--no-daemon"], { cwd: android, env: environment, stdio: "inherit" });
  child.on("exit", (code) => code === 0 ? resolvePromise() : reject(new Error(`Gradle exited ${code}`)));
});

const source = resolve(android, "app", "build", "outputs", "apk", "debug", "app-debug.apk");
const releases = resolve(root, "releases");
const output = resolve(releases, "SHAeR-Companion-0.19.0-debug.apk");
await mkdir(releases, { recursive: true });
await cp(source, output);
const digest = createHash("sha256").update(await readFile(output)).digest("hex");
await writeFile(`${output}.sha256`, `${digest}  ${output.split("/").pop()}\n`);
console.log(`${output}\n${digest}`);
