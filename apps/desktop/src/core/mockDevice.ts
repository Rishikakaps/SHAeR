export const MOCK_DEVICE_ID = "mock:shaer-desktop-dev";

export function createMockSnapshot() {
  return {
    mock: true,
    discovery: {
      device_id: MOCK_DEVICE_ID,
      device_name: "MOCK DEVICE - SHAeR",
      firmware_version: "mock-only",
      baseUrl: "mock://shaer"
    },
    dashboard: {
      device_name: "MOCK DEVICE - SHAeR",
      firmware_version: "mock-only",
      battery_percent: 72,
      charging: true,
      current_theme: "archive_dark",
      storage: { used: 2147483648, total: 8589934592, free: 6442450944 },
      now_playing: { title: "Mock-only track", artist: "Development", source: "mock" }
    },
    settings: {
      data: {
        display: { brightness: 70, sleep_timeout_s: 120, show_battery_percent: true, theme: "archive_dark" },
        device: { name: "MOCK DEVICE - SHAeR" }
      }
    },
    wifi: { supported: true, ssid: "Mock Wi-Fi", ip_address: "192.0.2.10", hostname: "mock-shaer", signal_percent: 88, saved_networks: ["Mock Wi-Fi"] },
    wifiScan: { supported: true, networks: [{ ssid: "Mock Wi-Fi", signal_percent: 88, security: "WPA2", current: true }] },
    bluetooth: { supported: true, enabled: true, discoverable: false, paired_devices: [{ id: "00:00:00:00:00:00", name: "Mock Headphones", state: "paired" }] },
    bluetoothScan: { supported: true, devices: [{ id: "00:00:00:00:00:00", name: "Mock Headphones", state: "discovered", kind: "audio" }] },
    music: { tracks: [] }
  };
}
