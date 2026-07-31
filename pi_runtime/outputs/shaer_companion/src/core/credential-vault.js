const DATABASE = "shaer-companion-vault";
const STORE = "credentials";
const RECORD = "active-device";
const KEY = "encryption-key";

function openDatabase() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DATABASE, 1);
    request.onerror = () => reject(request.error);
    request.onupgradeneeded = () => request.result.createObjectStore(STORE);
    request.onsuccess = () => resolve(request.result);
  });
}

function transaction(db, mode, operation) {
  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE, mode);
    const request = operation(tx.objectStore(STORE));
    request.onerror = () => reject(request.error);
    request.onsuccess = () => resolve(request.result);
    tx.oncomplete = () => db.close();
    tx.onerror = () => reject(tx.error);
  });
}

function nativePlugin() {
  return globalThis.Capacitor?.Plugins?.ShaerCredentialVault || null;
}

function encode(bytes) {
  return btoa(String.fromCharCode(...bytes));
}

function decode(value) {
  return Uint8Array.from(atob(value), (character) => character.charCodeAt(0));
}

async function webKey(db) {
  let key = await transaction(db, "readonly", (store) => store.get(KEY));
  if (key) return key;
  key = await crypto.subtle.generateKey({ name: "AES-GCM", length: 256 }, false, ["encrypt", "decrypt"]);
  const writable = await openDatabase();
  await transaction(writable, "readwrite", (store) => store.put(key, KEY));
  return key;
}

export class CredentialVault {
  constructor({ session = globalThis.sessionStorage } = {}) {
    this.session = session;
    this.memory = null;
  }

  async load() {
    const plugin = nativePlugin();
    if (plugin) {
      const result = await plugin.getCredential({ key: RECORD });
      return result?.value ? JSON.parse(result.value) : null;
    }
    if (!globalThis.indexedDB || !globalThis.crypto?.subtle) {
      const value = this.session?.getItem(RECORD);
      return value ? JSON.parse(value) : this.memory;
    }
    try {
      const db = await openDatabase();
      const encrypted = await transaction(db, "readonly", (store) => store.get(RECORD));
      if (!encrypted) return null;
      const keyDb = await openDatabase();
      const key = await webKey(keyDb);
      const plain = await crypto.subtle.decrypt({ name: "AES-GCM", iv: decode(encrypted.iv) }, key, decode(encrypted.ciphertext));
      return JSON.parse(new TextDecoder().decode(plain));
    } catch {
      return null;
    }
  }

  async save(credential) {
    const clean = {
      baseUrl: String(credential?.baseUrl || ""),
      token: String(credential?.token || ""),
      deviceId: String(credential?.deviceId || ""),
      deviceName: String(credential?.deviceName || "SHAeR")
    };
    const plugin = nativePlugin();
    if (plugin) {
      await plugin.setCredential({ key: RECORD, value: JSON.stringify(clean) });
      return;
    }
    if (!globalThis.indexedDB || !globalThis.crypto?.subtle) {
      this.memory = clean;
      this.session?.setItem(RECORD, JSON.stringify(clean));
      return;
    }
    const db = await openDatabase();
    const key = await webKey(db);
    const iv = crypto.getRandomValues(new Uint8Array(12));
    const ciphertext = new Uint8Array(await crypto.subtle.encrypt({ name: "AES-GCM", iv }, key, new TextEncoder().encode(JSON.stringify(clean))));
    const writable = await openDatabase();
    await transaction(writable, "readwrite", (store) => store.put({ iv: encode(iv), ciphertext: encode(ciphertext) }, RECORD));
  }

  async clear() {
    const plugin = nativePlugin();
    if (plugin) {
      await plugin.deleteCredential({ key: RECORD });
      return;
    }
    this.memory = null;
    this.session?.removeItem(RECORD);
    if (!globalThis.indexedDB) return;
    const db = await openDatabase();
    await transaction(db, "readwrite", (store) => store.delete(RECORD));
  }

  async migrateLegacy(baseUrl) {
    const legacy = globalThis.localStorage?.getItem("shaerCompanionToken");
    if (!legacy) return null;
    const credential = { baseUrl, token: legacy, deviceId: "", deviceName: "SHAeR" };
    await this.save(credential);
    globalThis.localStorage.removeItem("shaerCompanionToken");
    return credential;
  }
}
