# VE.Direct Unit-Test Strategy

The checksum implementation is not complete until automated tests verify its
behavior. Building the Pico firmware proves only that the code compiles.

## Test architecture

Extract the VE.Direct byte parser from `src/main.c` into a hardware-independent
module. The production firmware and the unit-test executable must compile the
same parser source; tests must not contain a separate copy of the algorithm.

The parser should accept one byte at a time and report events through a small
API, for example:

```c
void vedirect_parser_init(vedirect_parser_t *parser);
vedirect_result_t vedirect_parser_feed(
    vedirect_parser_t *parser,
    uint8_t byte,
    vedirect_measurement_t *measurement
);
```

Possible results are:

- `VEDIRECT_IN_PROGRESS`
- `VEDIRECT_VALID_BLOCK`
- `VEDIRECT_INVALID_CHECKSUM`
- `VEDIRECT_INCOMPLETE_BLOCK`

The parser module must not depend on Pico SDK headers, UART, OLED code, or USB.
This allows the tests to run as a normal macOS executable using the host C
compiler.

## Required test cases

1. Feed a known valid VE.Direct block and expect one valid-block event.
2. Corrupt one byte in that block and expect an invalid-checksum event.
3. Test checksum values outside printable ASCII, including `0x00`, CR, and LF.
4. Feed a truncated block and verify that no valid measurement is published.
5. Feed an invalid block followed by a valid block and verify parser recovery.
6. Feed multiple valid blocks and expect exactly one event per block.
7. Verify all required fields belong to the same block.
8. Verify a missing required field prevents publication.
9. Verify valid, invalid, and received counters independently.
10. Verify field lines or blocks longer than configured buffers are rejected
    safely without writing out of bounds.

## Fixtures

Keep raw byte arrays for test frames rather than C strings because the checksum
value is binary and may contain `0x00`. For each valid fixture, calculate the
checksum byte in the test setup:

```text
checksum_byte = (256 - (sum_of_all_previous_block_bytes % 256)) % 256
```

Then append that byte after `Checksum\t`. This verifies the parser with the full
range of possible checksum-byte values.

## Build integration

Add a host-only CMake test target and register it with CTest. It should be
possible to run all parser tests with:

```sh
cmake -S "unit test" -B "unit test/build"
cmake --build "unit test/build"
ctest --test-dir "unit test/build" --output-on-failure
```

Hardware testing remains a separate integration step: after unit tests pass,
flash the Pico and confirm its counters and output using a real VE.Direct
device.

## Completion rule

Mark checksum validation complete in `docs/next_steps.md` only after all parser
unit tests pass and the Pico firmware still builds successfully.
