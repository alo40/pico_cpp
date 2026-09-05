# Project instructions

## Architecture
- `src/main.c` is the active Pico application.
- `src/vedirect_parser.c` is hardware-independent parser logic.
- `include/vedirect_parser.h` is its public API.
- `unit test/test_vedirect_parser.c` tests the same parser source used by firmware.
- `src/ssd1306.c` and `include/ssd1306.h` contain Pico/OLED-specific display code.
- Do not copy files from `examples/` into `main.c`.

## Build workflow
- Firmware:
  `cmake -S . -B build -G Ninja`
  `cmake --build build`

- Unit tests:
  `cmake -S "unit test" -B "unit test/build"`
  `cmake --build "unit test/build"`
  `ctest --test-dir "unit test/build" --output-on-failure`

## Change policy
- Keep VE.Direct parser independent from Pico SDK headers and hardware-specific code.
- Public parser API changes normally require updates to:
  - `include/vedirect_parser.h`
  - `src/vedirect_parser.c`
  - relevant unit tests
- Internal `static` parser helpers should normally be tested indirectly through the public API.
- Do not flash automatically unless explicitly requested.
- After code changes, build firmware and run unit tests.
- Keep changes minimal and avoid unrelated refactoring.
- Show the relevant diff after modifying files.

## Documentation
- `docs/improvements.md` = long-term project backlog.
- `docs/next_steps.md` = active execution plan.
- Update documentation only after implementation and tests pass.
- Do not mark unrelated tasks complete.