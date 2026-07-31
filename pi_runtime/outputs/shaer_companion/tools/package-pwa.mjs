import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";
import { dirname, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const output = resolve(root, "releases", "shaer-companion-pwa-0.18.0.zip");
await import("node:fs/promises").then(({ mkdir }) => mkdir(dirname(output), { recursive: true }));
await new Promise((resolvePromise, reject) => {
  const process = spawn("zip", ["-qr", output, "."], { cwd: resolve(root, "dist"), stdio: "inherit" });
  process.on("exit", (code) => code === 0 ? resolvePromise() : reject(new Error(`zip exited ${code}`)));
});
const digest = createHash("sha256").update(await readFile(output)).digest("hex");
await writeFile(`${output}.sha256`, `${digest}  ${output.split("/").pop()}\n`);
console.log(`${output}\n${digest}`);
