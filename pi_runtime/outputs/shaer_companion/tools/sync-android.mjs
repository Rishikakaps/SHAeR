import { cp, mkdir, rm } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const destination = resolve(root, "android", "app", "src", "main", "assets", "public");
await rm(destination, { recursive: true, force: true });
await mkdir(destination, { recursive: true });
await cp(resolve(root, "dist"), destination, { recursive: true });
await cp(resolve(root, "capacitor.config.json"), resolve(root, "android", "app", "src", "main", "assets", "capacitor.config.json"));
console.log(`Synced shared web build to ${destination}`);
