<!-- 8ee545a9-e7cc-455f-9553-d7021642a929 048e8aec-8fb6-4d44-a30b-b4ed08d059ed -->
# v0.14.0 - ODR Hardening + LEAP Integration

## Phase 1: Repository Structure & Feature Flags

Create new directory structure and feature flag system:

- `config/features.h` - Compile-time feature toggles (ODR_HARDENED, SNAPSHOT_V2, LEAP_ON)
- `src/glue/feature_gates.cpp` - Runtime feature flag system reading env vars
- `include/melvin/odr/odr_contracts.h` - Pure-virtual ABI-safe interfaces (IReasoningEngine, IPathProver, IRelationPriors)
- `include/melvin/api/brain_io.h` - Modality-agnostic I/O interfaces (stubs for v0.15)
- `src/api/brain_io_stub.cpp` - No-op implementations (compile-time only)

Update directory structure to match `/include/melvin/{core,reasoning,learning,api,odr}/`.

## Phase 2: ODR Hardening

Fix One Definition Rule violations across the codebase:

- Move all inline non-trivial definitions from headers to `.cpp` files
- Ensure all globals/static objects are in translation units, not headers
- Add `export.h` with `MELVIN_EXPORT` macro for public API visibility
- Mark all symbols with proper visibility attributes (`-fvisibility=hidden` default)
- Organize all code under `namespace melvin::{core,reasoning,learning,api}`
- Create `tests/odr/test_odr_headers.cpp` to verify include-order independence (50 random permutations)
- Create `tests/odr/test_dupe_symbols.cpp` to verify no duplicate symbols when linking multiple TUs

## Phase 3: LEAP Integration Bridge

Integrate v0.13 LEAP system into v0.10 runtime via abstraction layer:

- `src/glue/reasoning_bridge.cpp` - Implements `IReasoningEngine` interface wrapping v0.13 `LeapEngine`
- Adapt CL loop calls: `infer(query, k, priors)` returns `(answer, path_proof, scores)`
- Expose relation priors from v0.13 with same default weights as golden tests
- Support multi-hop k-hop search (default k=3, read from `MELVIN_K` env var)
- Preserve exact v0.13 LEAP behavior (log-odds confidence, evidence calibration, hysteresis)

Update `src/main/melvin_runtime.cpp` to use `IReasoningEngine` instead of direct inference calls.

## Phase 4: Snapshot V2 (Backward Compatible)

Upgrade snapshot format with backward compatibility:

- `include/melvin/core/snapshot.h` - Snapshot V2 header structures
- `src/core/snapshot.cpp` - V2 writer with new `ReasoningState` section (beam params, priors seed, gate state)
- Add version byte + length-prefixed sections for forward safety
- V1 snapshot loader with auto-migration to V2 (synthesize default ReasoningState)
- `tests/storage/test_snapshot_v2.cpp` - Round-trip tests (V1→V2, V2→V1 graceful degrade)

## Phase 5: Continuous Learning Loop Wiring

Update CL runtime to use LEAP while preserving v0.10 behavior:

- Replace direct inference with `IReasoningEngine::infer()` calls
- Preserve SRS/Decay cadence and teaching→edges→decay ordering
- Log path proofs to metrics CSV when `LEAP_ON=true`
- Add new CSV columns: `leap_enabled`, `beam_k`, `path_len`, `top1_score`, `top2_margin`, `snapshot_version`
- Add console logging: `[LEAP] k=3 top1=0.82 margin=0.19 path=fire→smoke→clouds`

## Phase 6: Testing Suite

Comprehensive test coverage:

**Unit Tests:**

- `tests/reasoning/test_leap_merge.cpp` - Verify k-hop outputs match v0.13 goldens on 10 prompts
- `tests/reasoning/test_path_proof.cpp` - Verify path nodes/edges sequences are stable
- `tests/learning/test_srs_decay.cpp` - Verify SRS/decay unchanged vs v0.10 goldens

**E2E Tests:**

- `tests/e2e/test_continuous_learning_v14.cpp` - Run 300 ticks, inject 5 `.tch` files, assert:
- Memory growth < 5% above baseline
- Snapshot rotation works
- LEAP path proofs emitted
- SRS timings intact

**Build Tests:**

- Test on Debug/RelWithDebInfo/Release builds
- Test with clang and gcc compilers
- Run with `-fsanitize=address,undefined` in Debug mode
- Verify `-Wall -Wextra -Werror` passes

## Phase 7: Deployment & Rollback Scripts

Production deployment infrastructure:

- `scripts/build_all.sh` - Build all configurations (Debug/Release, clang/gcc)
- `scripts/run_e2e_v14.sh` - Run full E2E test suite
- `scripts/deploy_v14.sh` - Stop watcher, backup bin+snapshot, deploy new bin, smoke test (30s), restart with LEAP enabled
- `scripts/rollback_v14.sh` - Restore previous bin + last known-good snapshot, disable LEAP
- `docs/V014_CHANGELOG.md` - User-facing changes
- `docs/V014_MERGE_PLAN.md` - Technical integration details
- `docs/V014_ROLLBACK.md` - 30-second recovery procedure

## Phase 8: Observability & Metrics

Enhanced monitoring for production:

- Extend `continuous_learning_metrics.csv` with LEAP-specific columns
- Add structured log output for LEAP inference events
- Add snapshot version tracking
- Verify metrics are queryable and plottable

## Definition of Done

- [ ] All tests pass in Debug/RelWithDebInfo/Release (clang + gcc)
- [ ] E2E runs ≥1,200 ticks without crash/leak
- [ ] Golden answers match v0.13 outputs exactly
- [ ] CL behavior matches v0.10 within documented tolerances
- [ ] Snapshots V1/V2 cross-load verified
- [ ] Rollback verified on dry run
- [ ] No ODR violations detected
- [ ] Memory stable under continuous operation
- [ ] Feature flags work correctly (enable/disable LEAP at runtime)
- [ ] Documentation complete (changelog, merge plan, rollback guide)

### To-dos

- [ ] Create repository structure, feature flags system, and ABI-safe interface contracts
- [ ] Fix ODR violations: move inline definitions to .cpp, add visibility attributes, create ODR tests
- [ ] Implement reasoning bridge adapting v0.13 LEAP to IReasoningEngine interface
- [ ] Implement Snapshot V2 with backward compatibility and migration logic
- [ ] Wire LEAP into continuous learning runtime, add metrics logging, preserve v0.10 behavior
- [ ] Create comprehensive test suite: unit tests, E2E tests, ODR verification, build matrix
- [ ] Create deployment and rollback scripts with 30-second recovery procedure
- [ ] Add LEAP-specific metrics, structured logging, and monitoring dashboards
- [ ] Run full verification: all tests pass, 1200+ tick soak test, golden output validation, rollback dry run