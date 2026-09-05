# Solar Data Acquisition with Raspberry Pi Pico

This project acquires Victron MPPT data over VE.Direct, validates complete
measurement snapshots on a Raspberry Pi Pico, sends sequenced CSV over USB, and
stores timestamped data on macOS for Python analysis.

The current direction is analysis-led: firmware and protocol fields should be
extended when they answer a concrete engineering question or improve data
reliability.

See:

* [macOS data logging](docs/data_logging.md)
* [current workflow](docs/next_steps.md)
* [project backlog and priorities](docs/improvements.md)
