# Rust engine FFI safety contract

The Rust simulation remains authoritative, but every native host is responsible for honoring these process-level invariants.

## Pointer ownership

- `rust_engine_create` returns one unique allocation.
- `rust_engine_destroy` accepts that pointer exactly once.
- Null pointers are tolerated by the exported functions, but stale, forged, or concurrently mutated pointers are invalid.
- The `SurfacePatchView` pointer is immutable process-lifetime data and must never be freed by a host.

## Serialization

All operations on one `Engine` must be serialized by the host. The library does not add an internal lock because renderer event loops already own the scheduling boundary and a hidden lock would make callback re-entry prone to deadlock.

## Callback lifecycle

`rust_engine_tick` advances the simulation, snapshots the callback registration, ends the mutable Rust borrow, and only then invokes the host callback. A serialized callback may therefore call back into the C API without overlapping a live Rust mutable reference.

Callbacks must remain valid until cleared or replaced, must not unwind across the C ABI, and must not race engine destruction. Direct Rust callers receive an `EngineEvent` from `Engine::tick`; automatic callback delivery is an FFI-wrapper responsibility.

## Numeric input

Every external axis and delta-time value is checked for finiteness before it reaches simulation state. NaN and infinity are converted to zero, normalized axes are clamped to `[-1, 1]`, and frame deltas are clamped to `[0, 0.1]` seconds.

## Validation

CI pins Rust 1.85.0 and rejects formatting drift, clippy warnings, test failures, documentation warnings, and release-build failures. Regression tests cover non-finite input, callback delivery, and serialized callback re-entry.
