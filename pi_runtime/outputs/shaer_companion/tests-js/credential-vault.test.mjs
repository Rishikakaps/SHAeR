import assert from "node:assert/strict";
import test from "node:test";
import { CredentialVault } from "../src/core/credential-vault.js";

test("fallback credential storage is session-scoped and revocable", async () => {
  const values = new Map();
  const session = { getItem: (key) => values.get(key) || null, setItem: (key, value) => values.set(key, value), removeItem: (key) => values.delete(key) };
  const vault = new CredentialVault({ session });
  await vault.save({ baseUrl: "http://shaer.local:8775", token: "sensitive", deviceId: "one" });
  assert.equal((await vault.load()).token, "sensitive");
  await vault.clear();
  assert.equal(await vault.load(), null);
});
