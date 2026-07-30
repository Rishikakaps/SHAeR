import type { UserFacingError } from "../../../../packages/shared/src/desktop-contracts";

export class ShaerDesktopError extends Error {
  readonly code: string;
  readonly status: number;
  readonly dataChanged: boolean;
  readonly affected: string;

  constructor(message: string, options: { code?: string; status?: number; affected?: string; dataChanged?: boolean } = {}) {
    super(message);
    this.name = "ShaerDesktopError";
    this.code = options.code || "desktop_error";
    this.status = options.status || 0;
    this.affected = options.affected || "No local data was changed.";
    this.dataChanged = Boolean(options.dataChanged);
  }
}

export function toUserFacingError(error: unknown): UserFacingError {
  const err = error instanceof Error ? error : new Error(String(error));
  const code = error instanceof ShaerDesktopError ? error.code : "unknown_error";
  const status = error instanceof ShaerDesktopError ? error.status : 0;
  return {
    code,
    title: status === 401 ? "Pairing Required" : "SHAeR Needs Attention",
    whatHappened: err.message,
    affected: error instanceof ShaerDesktopError ? error.affected : "The requested operation did not complete.",
    dataChanged: error instanceof ShaerDesktopError ? error.dataChanged : false,
    nextStep: status === 401
      ? "Pair this computer with SHAeR again."
      : "Check the device connection and retry the action.",
    technicalDetails: code
  };
}

export function redactSecrets(value: string): string {
  return value
    .replace(/Bearer\s+[A-Za-z0-9._~+/=-]+/gi, "Bearer [redacted]")
    .replace(/(token|password|passphrase|secret|client_secret)["':=\s]+[^"',\s}]+/gi, "$1=[redacted]");
}
