import { normalizeBaseUrl, ShaerApiClient } from "./api-client.js";

function unique(values) {
  return [...new Set(values.filter(Boolean))];
}

export class DeviceDiscovery {
  constructor({ fetchImpl = globalThis.fetch, timeoutMs = 1400 } = {}) {
    this.fetchImpl = fetchImpl;
    this.timeoutMs = timeoutMs;
  }

  candidates(remembered = "") {
    const origin = globalThis.location && /^https?:$/.test(globalThis.location.protocol) ? globalThis.location.origin : "";
    return unique([remembered, origin, "http://shaer.local:8775", "http://shaer:8775"]).map(normalizeBaseUrl);
  }

  async scan(remembered = "") {
    const plugins = globalThis.Capacitor?.Plugins || {};
    const scans = [];
    if (plugins.ShaerDiscovery) scans.push(plugins.ShaerDiscovery.discover({ serviceType: "_shaer._tcp.", timeoutMs: Math.max(this.timeoutMs, 3500) }));
    if (plugins.ShaerBluetoothDiscovery) scans.push(plugins.ShaerBluetoothDiscovery.discover({ timeoutMs: Math.max(this.timeoutMs, 3500) }));
    const scanResults = await Promise.allSettled(scans);
    const nativeCandidates = scanResults.flatMap((result) => result.status === "fulfilled" ? result.value.devices || [] : []);
    const addresses = unique([...nativeCandidates.map((item) => item.baseUrl), ...this.candidates(remembered)]);
    const probes = addresses.map((baseUrl) => this.probe(baseUrl));
    const reachable = (await Promise.all(probes)).filter(Boolean);
    const transportByUrl = new Map(nativeCandidates.map((item) => [normalizeBaseUrl(item.baseUrl), item.transport || "local-network"]));
    return reachable.map((device) => ({ ...device, transport: transportByUrl.get(device.baseUrl) || "local-network" }));
  }

  async probe(baseUrl) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), this.timeoutMs);
    try {
      const client = new ShaerApiClient({ baseUrl, fetchImpl: this.fetchImpl });
      const device = await client.request("/api/v1/device/discovery", { signal: controller.signal });
      return { baseUrl: normalizeBaseUrl(baseUrl), ...device };
    } catch {
      return null;
    } finally {
      clearTimeout(timer);
    }
  }
}
