const CACHE = "shaer-companion-v0.18.0";
const SHELL = [
  "./",
  "index.html",
  "src/companion.css",
  "src/companion.js",
  "src/core/api-client.js",
  "src/core/credential-vault.js",
  "src/core/discovery.js",
  "src/core/models.js",
  "src/core/music-store.js"
].map((path) => new URL(path, self.registration.scope).pathname);

self.addEventListener("install", (event) => event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(SHELL))));
self.addEventListener("activate", (event) => event.waitUntil(caches.keys().then((keys) => Promise.all(keys.filter((key) => key !== CACHE).map((key) => caches.delete(key))))));
self.addEventListener("fetch", (event) => {
  const url = new URL(event.request.url);
  if (event.request.method !== "GET" || url.pathname.startsWith("/api/")) return;
  event.respondWith(fetch(event.request).then((response) => {
    if (response.ok && url.origin === self.location.origin) caches.open(CACHE).then((cache) => cache.put(event.request, response.clone()));
    return response;
  }).catch(() => caches.match(event.request)));
});
