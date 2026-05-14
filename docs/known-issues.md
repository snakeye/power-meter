## Known issues

This file tracks confirmed firmware/hardware behavior problems with measurable impact.

---

### PM-FW-001 - Incorrect charge integration formula

- Severity: Critical
- Status: Fixed, pending bench validation
- Affected files: `firmware/src/adc.cpp`

Symptom:

- Accumulated charge grows incorrectly or almost does not grow for low currents.

Root cause:

- Firmware currently uses `current / tsDiff` instead of `current * tsDiff` in the charge accumulator.

Why this is incorrect:

- Physical relation is `dQ = I * dt`.
- Dividing by time computes a rate-like term, not charge increment.

Impact:

- Capacity (`mAh`) display is mathematically invalid.
- Result depends heavily on sample interval and can collapse near zero.

Fix direction:

- Replace integration step with multiplication: `Q += I * dt`.
- Keep units explicit and consistent (`uA`, `us`, `uA*us`).

Fix applied:

- `firmware/src/adc.cpp` now accumulates as `totalCharge += uint64_t(current) * uint64_t(tsDiff)`.
- Commit: `a1aa619`.

Acceptance criteria:

- Charge accumulation remains stable when ADC data rate (SPS) changes.
- Low current load (for example `~0.5 mA`) produces monotonic non-zero accumulation over time.

---

### PM-FW-002 - Wrong conversion factor from `uA*us` to `mAh`

- Severity: Critical
- Status: Fixed, pending bench validation
- Affected files: `firmware/src/display.cpp`

Symptom:

- Displayed `mAh` is scaled incorrectly by several orders of magnitude if `totalCharge` is in `uA*us`.

Root cause:

- Current divisor (`3600 * 1000`) corresponds to `uA*s -> mAh`, not `uA*us -> mAh`.

Correct conversion:

- `1 mAh = 3,600,000,000,000 uA*us`.

Fix direction:

- Use `mAh = totalCharge / 3600000000000ULL` when `totalCharge` is kept in `uA*us`.

Fix applied:

- `firmware/src/display.cpp` now uses `uA*us` scaling and displays `mAh` with one decimal using integer math (`deci-mAh`).
- Commit: `a1aa619` (+ post-commit UI precision update).

Acceptance criteria:

- Calculated capacity matches reference integration from logged current/time pairs.

---

### PM-FW-003 - Incorrect `printf` format for 64-bit value

- Severity: High
- Status: Fixed, pending bench validation
- Affected files: `firmware/src/display.cpp`

Symptom:

- Capacity output may appear valid in simple cases but is type-unsafe and can print corrupted values.

Root cause:

- `totalCharge` is `uint64_t`, but code uses `%d` which expects `int`.
- This is undefined behavior for variadic formatting.

Impact:

- Output can be platform/compiler dependent and break with argument layout changes.

Fix direction:

- Format with a type-compatible specifier or convert to a safe display type first.
- Recommended display path: compute `uint32_t mAh` after division, print as unsigned long-compatible argument.

Fix applied:

- Display path now prints pre-scaled 32-bit values with unsigned formatting.
- Commit: `a1aa619`.

Acceptance criteria:

- Output remains correct with multiple arguments in a single `printf` call.
- No compiler warnings for format/type mismatch.

---

### PM-FW-004 - Integer truncation sensitivity in old algorithm

- Severity: High
- Status: Fixed
- Affected files: `firmware/src/adc.cpp`

Symptom:

- Small-current contribution disappears due to integer division truncation.

Root cause:

- In the old division-based formula, fractional parts are discarded at each sample.

Impact:

- Strong underestimation of capacity, worst at low currents.

Fix direction:

- Remove division-based integrator.
- Preserve precision in 64-bit intermediate math when multiplying `current * tsDiff`.

Fix applied:

- Division-based integration removed.
- 64-bit multiplication is used for charge accumulation.
- Commit: `a1aa619`.

Acceptance criteria:

- Numerical simulation with fractional-step contributions no longer loses low-current energy over time.

---

## Verification checklist after fixes

- Test at low load (`~0.5 mA`): `mAh` must increase slowly but steadily.
- Test at moderate load (`~50-100 mA`): displayed accumulation should track expected runtime capacity.
- Repeat with at least two ADS1220 data rates: final integrated `mAh` should remain consistent within normal measurement error.
- Confirm no format warnings from compiler for display print calls.
