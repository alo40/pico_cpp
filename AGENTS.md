# Project instructions

## Canonical documentation

- `README.md` is the human entry point and quick-start document.
- `docs/PROJECT.md` is the canonical technical project document. It owns:
  - system architecture,
  - component responsibilities,
  - data flow and data model,
  - implemented capabilities,
  - verification status,
  - known limitations,
  - active work.
- `docs/COMMANDS.md` is an operational command reference only.
- Do not create parallel roadmap, next-steps, architecture, or status documents unless explicitly requested.
- Keep documentation consistent with the actual repository and implementation.

## Firmware architecture

- `src/main.c` is the active Pico application.
- `src/vedirect_uart.c` owns Pico UART transport and parser polling.
- `src/dashboard.c` owns Pico/OLED dashboard state and rendering.
- `src/vedirect_parser.c` is hardware-independent parser logic.
- `include/vedirect_parser.h` is the parser public API.
- `unit test/test_vedirect_parser.c` tests the same parser source used by firmware.
- `src/ssd1306.c` and `include/ssd1306.h` contain Pico/OLED-specific display code.
- Do not copy files from `examples/` into `main.c`.
- Keep Pico SDK headers and hardware-specific behavior out of `vedirect_parser`.

## Host architecture

- `scripts/log_vedirect.py` owns persistent Raspberry Pi logging.
- The logger is the writer of raw and processed measurement files.
- `scripts/dashboard.py` is a read-only consumer of processed CSV files.
- The dashboard must not access the Pico serial device or modify measurement files.
- `scripts/sync_data.sh` performs Raspberry Pi-to-Mac synchronization.
- Jupyter notebooks analyze synchronized local files only and must not initiate network synchronization.

## Data contract

Processed CSV schema:

```csv
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Rules:

- Preserve the timezone-aware Raspberry Pi logger timestamp by default.
- Do not describe it as an MPPT or Pico hardware timestamp.
- Treat Pico sequence numbers as monotonic only within one firmware execution.
- Battery/panel voltage is stored in mV, battery current in mA, and panel power in W.
- Positive battery current means charging; negative means discharging.
- Do not present irradiance, temperature, or accurate battery state of charge as directly measured values.

## Build workflow

Firmware:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Tests:

```sh
cmake -S "unit test" -B "unit test/build"
cmake --build "unit test/build"
ctest --test-dir "unit test/build" --output-on-failure
python3 -m unittest discover -s "unit test" -p "test_*.py"
```

## Change policy

- Keep changes minimal and scoped to the requested task.
- Do not perform unrelated refactoring.
- Public parser API changes normally require updates to:
  - `include/vedirect_parser.h`,
  - `src/vedirect_parser.c`,
  - relevant tests.
- Internal `static` parser helpers should normally be tested indirectly through the public API.
- Do not flash hardware automatically unless explicitly requested.
- After firmware/code changes, build the affected software and run relevant tests.
- Do not overwrite or revert unrelated pre-existing working-tree changes.
- Inspect and report the relevant diff after modifying files.

## Documentation policy

- Update `docs/PROJECT.md` only after implementation and relevant tests/verification pass.
- Keep implemented capability, verification status, and active work distinct inside `docs/PROJECT.md`.
- An implemented feature may still have an open real-hardware verification item.
- Do not mark unrelated work complete.
- Keep `docs/COMMANDS.md` concise; it should contain commands, not architecture or project history.
