export type ShaerCapabilityKey =
  | "externalSsd"
  | "wifiManagement"
  | "bluetoothAudio"
  | "recorder"
  | "spotifyApi"
  | "spotifyConnect"
  | "themeEditing"
  | "batteryMonitor"
  | "annotations"
  | "backups"
  | "advancedAudioSettings";

export type ShaerCapabilities = Record<ShaerCapabilityKey, boolean>;

export type PairingState = "idle" | "requested" | "waiting" | "paired" | "denied" | "expired" | "cancelled";
export type ConnectionState = "offline" | "discovering" | "pairing" | "online" | "unauthorised" | "mock";
export type TransferState = "waiting" | "preparing" | "transferring" | "verifying" | "complete" | "failed" | "cancelled" | "paused";
export type SyncState = "idle" | "checking" | "changes_detected" | "syncing" | "verifying" | "complete" | "partial_success" | "failed" | "conflict";

export interface DeviceIdentity {
  id: string;
  name: string;
  model?: string;
  hardwareRevision?: string;
  firmwareVersion?: string;
  osVersion?: string;
  ipAddress?: string;
  baseUrl: string;
  connectionType: "local-network" | "manual" | "bluetooth-bootstrap" | "mock";
  paired: boolean;
  trusted: boolean;
  online: boolean;
  lastSeenIso?: string;
}

export interface DeviceStatus {
  batteryPercent?: number | null;
  charging?: boolean | null;
  storageUsedBytes?: number;
  storageTotalBytes?: number;
  activeWifiSsid?: string;
  bluetoothEnabled?: boolean;
  activeAudioOutput?: string;
  currentTheme?: string;
  currentTrack?: string;
  libraryTrackCount?: number;
  lastSyncIso?: string;
  warnings: string[];
}

export interface StorageVolume {
  id: string;
  label: string;
  kind: "internal" | "ssd" | "usb" | "unknown";
  mounted: boolean;
  totalBytes: number;
  usedBytes: number;
  defaultForMusic: boolean;
}

export interface MusicTrack {
  id: string;
  title: string;
  artist: string;
  album?: string;
  durationMs?: number;
  format?: string;
  bitrateKbps?: number;
  fileSizeBytes?: number;
  source: "desktop" | "shaer" | "spotify" | "unknown";
  destination?: string;
  duplicateState?: "unique" | "duplicate" | "possible_duplicate";
  syncState?: "local_only" | "device_only" | "synced" | "changed";
  profileId?: string;
}

export interface TransferJob {
  id: string;
  filename: string;
  destination: string;
  state: TransferState;
  bytesTotal: number;
  bytesDone: number;
  speedBytesPerSecond?: number;
  failureReason?: string;
  checksum?: string;
}

export interface WifiNetwork {
  ssid: string;
  signalPercent?: number;
  security: "open" | "wpa2" | "wpa3" | "enterprise" | "unknown";
  saved: boolean;
  current: boolean;
  internet?: boolean | null;
}

export interface BluetoothDevice {
  id: string;
  name: string;
  kind: "audio" | "phone" | "computer" | "input" | "unknown";
  state: "discovered" | "pairing" | "paired" | "connecting" | "connected" | "rejected" | "unavailable";
  supportedProfile?: string;
}

export interface ShaerThemeSummary {
  id: string;
  name: string;
  builtIn: boolean;
  active: boolean;
  editable: boolean;
}

export interface SyncConflict {
  id: string;
  category: string;
  label: string;
  desktopModifiedIso: string;
  deviceModifiedIso: string;
}

export interface DiagnosticItem {
  id: string;
  label: string;
  state: "ok" | "warning" | "failed" | "unknown";
  detail: string;
}

export interface DesktopRegistry {
  selectedDeviceId?: string;
  devices: DeviceIdentity[];
  transfers: TransferJob[];
  lastSyncReport?: string;
}

export interface UserFacingError {
  code: string;
  title: string;
  whatHappened: string;
  affected: string;
  dataChanged: boolean;
  nextStep: string;
  technicalDetails?: string;
}
