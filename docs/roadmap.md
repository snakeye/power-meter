## Roadmap

This roadmap reflects the current codebase state and practical next steps.

## Near-term priorities

1. Implement configuration persistence
   - Complete `loadConfig()` / `saveConfig()` with EEPROM layout and CRC validation.
   - Add safe defaults and versioning for future config changes.

2. Validate and fix charge accumulation
   - Revisit integration math and target units.
   - Add tests or offline validation scripts with known waveforms.

3. Complete input model
   - Define button B function (for example mode switch, calibration, or hold/reset behavior).
   - Add user feedback on screen for state changes.

4. Improve ISR/main-loop data consistency
   - Introduce snapshot logic or lock-free pattern for coherent display reads.
   - Reduce risk of partial multi-variable reads during rendering.

## Mid-term improvements

- Calibration workflow for zero offset and scaling factors
- Error detection and recovery (ADC init/read failures)
- Optional serial telemetry export for logging
- Power and noise characterization documentation

## Public-project readiness

- Add LICENSE file
- Add CONTRIBUTING guide
- Add issue templates for bug reports and feature requests
- Add hardware revision and tested setup matrix
