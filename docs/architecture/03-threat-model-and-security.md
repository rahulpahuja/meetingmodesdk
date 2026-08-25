# Threat Model & Security Architecture

See `05-diagrams.md` §10 for the visual trust-boundary diagram this document narrates.

## 1. Trust boundaries

1. **Device OS/app sandbox boundary** — the outermost boundary; everything below is trusted at
   the app's own privilege level.
2. **C++ core SDK boundary** — in-process, same trust level as the host app, but architecturally
   isolates domain logic so it can be reasoned about independently of platform code.
3. **On-device model boundary** — bundled model files; the concern here is *integrity* (a
   tampered model producing wrong/malicious output), not confidentiality.
4. **Local encrypted storage boundary** — the meeting database and audio segment files at rest.
5. **Optional cloud-provider boundary** — network egress. The only boundary that is **off by
   default** and requires explicit, per-feature user consent to cross.

## 2. Assets and sensitivity

| Asset | Sensitivity |
|---|---|
| Raw microphone audio | Highest — directly reconstructs conversation content. |
| Transcripts | High — full semantic content of the meeting. |
| Speaker embeddings/voiceprints | High — biometric-adjacent; treat with the same handling rigor as raw audio, not as "just metadata." |
| Summaries, action items, decisions | Medium-high — derived, but still expose meeting substance. |
| Search index / embeddings | Medium-high — vector embeddings can leak source content (inversion attacks exist); must be encrypted equivalently to the transcript they're derived from, not treated as "just numbers." |
| Encryption keys | Critical — compromise invalidates every other protection. |

## 3. Threat model

| # | Vector | Asset | Mitigation |
|---|---|---|---|
| a | Another app or malware reads meeting data off disk | Transcripts, audio, embeddings | Encryption at rest with OS-keystore-backed keys (§5); plaintext never touches disk outside the encrypted store. |
| b | Accidental cloud upload when user never enabled a cloud provider | Raw audio, transcript | `AIProviderConfig` defaults every slot to `Disabled`/`OnDevice`; only `providers/cloud` may perform network I/O (module-boundary enforced, not just convention — `speech`/`speaker`/`translation`/`intelligence` never link a network stack); no code path exists to reach a network call without going through a `ProviderMode::Cloud`-configured slot. |
| c | Sensitive content leaks through logs | Transcript, audio, PII | `ILogger::log` takes typed `LogField{key,value}` pairs, not free-text — there is no API surface a caller can use to pass a raw transcript string into the log path by accident (see `02-interfaces-and-data-models.md` §3). Production log level defaults to `Warn`. |
| d | Encryption key compromise | All stored data | Keys held in OS-backed secure storage (Android Keystore / iOS Keychain-Secure Enclave), never serialized alongside the ciphertext they protect, never held in application-readable form longer than the operation requires (§5). |
| e | Stale data survives after user deletes a meeting | Transcript, audio, embeddings | `IMeetingRepository::remove` is defined as *crypto-erase*: discard the meeting's data-encryption key, not merely unlink files — recoverable-by-undelete plaintext is not an acceptable outcome (§5). |
| f | A malicious or compromised cloud provider implementation added later | Whatever data reaches it | Each `providers/cloud` implementation receives only the minimum data slice the interface contract allows (e.g. `ITranslator::translate` receives one text string, never a `Meeting`); per-slot consent (§6) means enabling cloud LLM cannot cause audio to reach a cloud STT provider — there is no shared "cloud mode" flag to compromise once. |

## 4. Data lifecycle

| Stage | Trigger | Encrypted? | Keys | Default retention |
|---|---|---|---|---|
| Capture (in-memory) | User starts recording | No (RAM only, ephemeral) | n/a | Cleared on stop/crash; never written to unencrypted disk/swap-sensitive buffers minimized. |
| Processing (in-memory) | Pipeline runs | No (RAM only) | n/a | Exists only for the duration of pipeline execution. |
| Persisted transcript/audio/embeddings | Meeting saved | Yes, at rest | Per-meeting DEK wrapped by device master key (§5) | Until user deletes, or a configured retention policy expires it (§15 requires this be user-configurable; default = keep until explicit deletion). |
| Optional cloud call | Only if the relevant `AIProviderConfig` slot is `Cloud` | In transit: TLS; at the provider: outside this system's boundary | n/a (provider-side) | Governed by the cloud provider's own retention — must be disclosed to the user before consent is granted. |
| Deletion | User deletes a meeting | N/A — crypto-erase | Per-meeting DEK discarded | Immediate; ciphertext may persist on disk but is unrecoverable without the discarded key. |

## 5. Encryption boundaries and key lifecycle

- **Key hierarchy**: a device-bound master key lives in the OS keystore (Android Keystore /
  iOS Secure Enclave-backed Keychain) and never leaves it in exportable form. It wraps a
  per-meeting (or per-database, if per-meeting proves too granular in practice) data-encryption
  key (DEK), which is what actually encrypts the meeting's transcript/audio/embedding records.
- **Rotation/backup posture**: keys are **not synced off-device by default**. This is a deliberate
  product tradeoff — syncing keys off-device (e.g. to enable cross-device meeting access) would
  introduce a new trust boundary (a key-backup service) that the current threat model does not
  cover. Flagging this explicitly for product/user confirmation before any multi-device feature is
  built on top of this architecture.
- **Secure deletion** means crypto-erase: discard the DEK so ciphertext becomes permanently
  unrecoverable, rather than relying on filesystem unlink (which can leave recoverable data on
  flash storage). `IMeetingRepository::remove` is specified to have this behavior, not "delete the
  row."

## 6. Cloud-provider trust boundary

The `ProviderMode { OnDevice, Cloud, Disabled }` tri-state from the translation design (see
`02-interfaces-and-data-models.md`) generalizes to **every** provider family in
`AIProviderConfig` — STT, LLM, translation, embeddings, diarization each have their own
independent mode.

**Invariant**: enabling one cloud provider slot must never implicitly enable another. Granting
cloud consent for summarization (LLM) must not cause audio to start flowing to a cloud STT
provider — each slot's consent is checked independently at the point that slot's provider is
constructed in `orchestration`, and there is no global "cloud enabled" flag that could
accidentally gate multiple slots at once.

## 7. See also

- `01-requirements-and-architecture.md` — module boundaries that make (b) and (f) above structural rather than convention-based.
- `02-interfaces-and-data-models.md` — `ILogger`, `AIProviderConfig`, `ProviderMode`, error taxonomy.
- `05-diagrams.md` §10 — the trust-boundary diagram.
