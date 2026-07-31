import assert from "node:assert/strict";
import test from "node:test";
import { ShaerApiClient, ShaerApiError, normalizeBaseUrl } from "../src/core/api-client.js";

test("normalizes local device addresses", () => {
  assert.equal(normalizeBaseUrl("shaer.local:8775/"), "http://shaer.local:8775");
});

test("unwraps a versioned success envelope and sends a bearer token", async () => {
  let authorization = "";
  const client = new ShaerApiClient({ baseUrl: "http://shaer.local:8775", tokenProvider: async () => "secret", fetchImpl: async (_url, options) => {
    authorization = options.headers.Authorization;
    return new Response(JSON.stringify({ ok: true, data: { ready: true } }), { status: 200 });
  } });
  assert.deepEqual(await client.request("/api/v1/dashboard"), { ready: true });
  assert.equal(authorization, "Bearer secret");
});

test("distinguishes network, parse, HTTP, and authentication failures", async () => {
  const network = new ShaerApiClient({ fetchImpl: async () => { throw new TypeError("offline"); } });
  await assert.rejects(network.request("/api"), (error) => error instanceof ShaerApiError && error.kind === "network");
  const parse = new ShaerApiClient({ fetchImpl: async () => new Response("not-json", { status: 200 }) });
  await assert.rejects(parse.request("/api"), (error) => error.kind === "parse");
  const auth = new ShaerApiClient({ fetchImpl: async () => new Response(JSON.stringify({ ok: false, error: { code: "invalid_token", message: "Revoked" } }), { status: 401 }) });
  await assert.rejects(auth.request("/api"), (error) => error.kind === "authentication" && error.code === "invalid_token");
});

test("latest requests cancel stale work", async () => {
  const client = new ShaerApiClient({ fetchImpl: (_url, options) => new Promise((_resolve, reject) => {
    if (options.signal.aborted) reject(new DOMException("cancelled", "AbortError"));
    else options.signal.addEventListener("abort", () => reject(new DOMException("cancelled", "AbortError")), { once: true });
  }) });
  const first = client.latest("search", "/one");
  const second = client.latest("search", "/two");
  await assert.rejects(first, (error) => error.kind === "cancelled");
  client.controllers.get("search").abort();
  await assert.rejects(second, (error) => error.kind === "cancelled");
});
