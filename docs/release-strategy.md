# UVHTTP Release Strategy

## Release cadence

**Release on demand.** Ship only when there is something to release and every gate passes. A week with no new content skips the release; the project does not force one version per week.

## Version rules

SemVer 2.0: `MAJOR.MINOR.PATCH`

| Version type | Trigger | Example |
|---------|---------|---------|
| **Patch (PATCH)** | Tests, docs, bug fixes | 2.5.0 → 2.5.1 |
| **Minor (MINOR)** | New features (backward compatible) | 2.5.0 → 2.6.0 |
| **Major (MAJOR)** | Incompatible API changes | 2.5.0 → 3.0.0 |

The version number follows **actual compatibility**, not the name a work window had when it started. When a window called "x.y minor" ends up merging incompatible changes, the release takes a MAJOR number; "the window was labelled MINOR" is not a reason to keep the decimal. That is where v3.0.0 came from: the window was developed as "v2.10" throughout (see brain page `api-atomization-orthogonalization`) and renumbered at release under the table above. The earlier "compatibility strategy B: change 2.10 destructively without a major bump" **skip-number exemption is revoked**. The other half of strategy B still holds: no deprecated compatibility layer and no parallel old and new APIs.

## Release process

> **Branch policy**: every change to main, release commits included, goes through a PR. Direct pushes to main are forbidden.

### Phase one: pre-release

1. **Full test pass**
   - `make test` (Debug 119/119)
   - `make verify-memory-safety` (ASan + UBSan)
   - `cd docs && npm run docs:build`

2. **Version preparation**
   - Update the `VERSION` file
   - Update `docs/guide/CHANGELOG.md`
   - Merge into main through a PR (PR only, no direct pushes)
   - Run `scripts/ci/release_checklist.sh --pre`: proceed only when every locally verifiable item passes

3. **Create the pre-release**
  - Create a git tag: `git tag v2.x.y`
  - Push the tag: `git push origin v2.x.y`
  - Create a pre-release: `gh release create v2.x.y --prerelease`
  - **Two gates start automatically**:
     - `ci-release-gate.yml` (full tests: fast tests / configuration matrix tests / ASan memory safety)
     - `ci-benchmark.yml` (performance regression gate: same-machine paired runs, 10% threshold)
  - Move to the final release only when both gates are green
  - The trigger is `release: types: [published]`. It **must be `published`**: that type covers both final and pre-releases. `prereleased` does not fire for a pre-release published from a draft, and `gh release create --prerelease` takes that path (for v3.0.0 neither gate had a run); `prerelease` is not a valid type, so GitHub accepts it without error and never matches (both gates were silent from 2026-10-16). The next step is therefore to confirm the gates fired, not to wait for them.
  - **A release event uses the workflow files on the tagged commit**, not on the default branch. Before tagging, confirm that both workflow triggers work on that commit. Editing main does not help a tag that already carries a broken trigger; retag instead (v3.0.0: the tag's `types: [prerelease]` meant the event produced no run even when sent, and only a retag onto the fixed commit fired the gates).

### Phase two: final release

4. **Confirm the gates**
   - Run `scripts/ci/release_checklist.sh --gates`: it looks up `release` event runs for both gates on the commit and tag name at the **remote** tag, using the latest run (a cancelled run counts as red until a rerun makes it latest); no run counts as red. **Check this before reading colors**: "all green" means nothing when there are no runs. On failure or cancellation, rerun with `gh run rerun <run-id>` (a rerun is a new attempt of the same run and leaves no extra run). Do not move the tag: moving it changes what was published and can fake a pass from an old run on the same commit.
   - Confirm the `ci-release-gate` test gate is green (full tests: fast / configurations / ASan memory safety)
   - Confirm the benchmark regression gate is green (triggered by the `release` event that creates the pre-release; during development, run `workflow_dispatch` manually to see trends, and manual runs only report, they do not fail. The project has no pre-release branch path, so PR CI has no automatic gate here.)
   - The gate compares paired head/base measurements on the same runner (it fails when the median paired ratio is below 90% and most pairs are also below 90%). Absolute RPS baselines are report information only (GitHub runner variance across runs is about 40%, so an absolute threshold would gate on runner luck, not code).

5. **Promote to final**
   - `gh release edit v2.x.y --latest` (removes the prerelease flag)
   - Merging to main through a PR triggers the docs deployment
   - Run `scripts/ci/release_checklist.sh --post`: the release is done only when the tag, the latest release and the live versions.json all pass
   - Confirm the site updated

## Release checklist

- [ ] Local `make test` passes (Release full suite; Debug is covered by the pre-release gate)
- [ ] `make verify-memory-safety` passes (ASan + UBSan)
- [ ] Docs build passes
- [ ] CHANGELOG updated
- [ ] VERSION updated
- [ ] Git tag created and pushed
- [ ] Pre-release created (--prerelease)
- [ ] **`release_checklist.sh --gates` passes** (tag name and commit match; the latest release run succeeded)
- [ ] **`ci-release-gate` test gate passes** (fast / configurations / ASan memory safety)
- [ ] Benchmark regression gate passes (CI / gate green)
- [ ] Final release confirmed (--latest)
- [ ] Site deployed

## Version history

Version history, the CHANGELOG and `versions.json` record the **tag date**. A GitHub Release `publishedAt` is later (pre-releases are deleted and recreated, and a release becomes latest only after the gates pass), so the two need not match (v3.0.0: tag on 2026-10-18, latest on 2026-10-20).

| Version | Date | Main changes |
|------|------|---------|
| v3.0.0 | 2026-10-18 | API atomization window (developed as "v2.10" throughout, renumbered at release under the table above): **removed** 14 never-read fields from `uvhttp_config_t` and 4 fake-knob setters (#472), the builder layer, `uvhttp_serve` and the verb functions (#479), the built-in rate limiter's 7 APIs and 8 server fields (#483), and getters without the request prefix (#470); **renamed** `set_router`/`set_context` to `take_router`/`take_context`, putting ownership in the name (#470); **added** `uvhttp_server_listen_routes` (#470) and the pre-dispatch seam `uvhttp_server_set_request_filter` (#483); **fixed** router migration destroying registered routes on failure (#471, data loss since v2.7.2), `set_body` not atomic (#473), LRU setter write-point validation (#473/#474), construction-path atomicity and truthful error codes (#475), framing misalignment when a manual `Content-Length` meets compression (#464, data corruption), and prewarm leaking one buffer per successful file (#449); **tightened** empty header names are no longer accepted (#457, RFC 7230 §3.2); also includes the post-2.9.2 test-validity audit and bringing `STATIC_FILES`/`ROUTER_CACHE` into the sanitizer build (#448/#453–#457/#468) |
| v2.9.2 | 2026-10-02 | Behavior tests: 11 groups, 132 tests (TLS certificates / URL encoding / conditional requests / gzip and LRU cache boundaries / response header growth / resumed writes across reads / query parameters / router trie limits / connection timeouts / accept count pairing, all mutation-verified); library version now injected from the VERSION file; fixed a test that could never fail; removed the format-check gate and ci-daily; trimmed ROADMAP |
| v2.9.1 | 2026-10-01 | CI gate fixes (format-check no-op + pinned clang-format version + trend workflow moved to a branch PR), least-privilege CI permissions, dead code cleanup |
| v2.9.0 | 2026-10-01 | Fuzz coverage expansion (request/websocket/static_path harnesses), header name control-character check (response splitting defense), integration→manual/ test infrastructure cleanup |
| v2.8.1 | 2026-09-29 | Same-machine paired gate, zero-copy small-body threshold fix |
| v2.8.0 | 2026-09-23 | Zero-copy writev (/large +72.7%), 17 fixes from the second review round |
| v2.7.0 | 2026-08-21 | TLS session cache, CI performance benchmarks, Platinum tier |
| v2.6.1 | 2026-08-12 | Docs sync, CI fixes and memory safety |
| v2.6.0 | 2026-08-03 | Health check, SSE and testability |
| v2.5.1 | 2026-07-24 | Test coverage increase + spec completion |
| v2.5.0 | 2026-03-15 | Version number update |
| v2.4.4 | 2026-02-26 | Release notes |
| v2.4.3 | 2026-02-26 | Merged pre-release: memory leak fix + docs fixes |
| v2.4.2 | 2026-02-25 | Version number update |
