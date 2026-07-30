import type { StoredCredential } from "./api";

const KEY = "shaer.realDeviceCredential.v1";
const MOCK_KEY = "shaer.mockDeviceState.v1";

export async function saveCredential(credential: StoredCredential): Promise<void> {
  localStorage.setItem(KEY, JSON.stringify(credential));
}

export async function loadCredential(): Promise<StoredCredential | null> {
  const raw = localStorage.getItem(KEY);
  if (!raw) return null;
  try {
    const value = JSON.parse(raw);
    if (value && typeof value.baseUrl === "string" && typeof value.token === "string") return value;
  } catch {
    localStorage.removeItem(KEY);
  }
  return null;
}

export async function clearCredential(): Promise<void> {
  localStorage.removeItem(KEY);
}

export function saveMockState(value: unknown): void {
  if (!import.meta.env.DEV) return;
  localStorage.setItem(MOCK_KEY, JSON.stringify(value));
}

export function loadMockState<T>(fallback: T): T {
  if (!import.meta.env.DEV) return fallback;
  const raw = localStorage.getItem(MOCK_KEY);
  if (!raw) return fallback;
  try {
    return JSON.parse(raw) as T;
  } catch {
    return fallback;
  }
}
