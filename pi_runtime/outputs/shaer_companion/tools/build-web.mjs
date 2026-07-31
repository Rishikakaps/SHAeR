import { cp, mkdir, rm } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const dist = resolve(root, "dist");
await rm(dist, { recursive: true, force: true });
await mkdir(dist, { recursive: true });
for (const name of ["index.html", "manifest.webmanifest", "service-worker.js", "src", "icons"]) {
  await cp(resolve(root, name), resolve(dist, name), { recursive: true });
}
console.log(`Built ${dist}`);
