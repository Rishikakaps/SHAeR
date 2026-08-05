# SHAeR Backup Format v1

Files use the `.shaer-backup` extension and this binary layout:

1. ASCII `SHAERBACKUP1` (12 bytes)
2. random scrypt salt (16 bytes)
3. random stream nonce (16 bytes)
4. HMAC-SHA-256 authentication tag (32 bytes)
5. encrypted ZIP payload

The passphrase is processed with scrypt (`N=16384`, `r=8`, `p=1`, 64-byte output). The first 32 bytes key an HMAC-SHA-256 counter stream; the second 32 bytes authenticate the header and ciphertext. Authentication is checked before ZIP parsing.

The ZIP contains `manifest.json` and selected data:

- `settings/settings.json`
- `library/library.db`
- `music/...`
- `themes/shaer_*/...`
- `artwork/...`

Restore is selective and rejects paths outside configured SHAeR data roots. Passphrases are never stored. This standard-library construction is versioned for prototype portability; a future format revision may migrate to a platform AES-GCM or ChaCha20-Poly1305 provider while retaining v1 restore support.
