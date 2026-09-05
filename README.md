# Solar Data Acquisition with Raspberry Pi Pico

This project acquires Victron MPPT data over VE.Direct, validates complete
measurement snapshots on a Raspberry Pi Pico, sends sequenced CSV over USB, and
stores timestamped data persistently on a Raspberry Pi. A Mac synchronizes
selected data over SSH for local Python analysis and visualization.

The Raspberry Pi is the always-on storage host. The Mac is not required for
continuous acquisition: its responsibilities are remote administration, data
synchronization, and analysis of local CSV copies.

The current direction is analysis-led: firmware and protocol fields should be
extended when they answer a concrete engineering question or improve data
reliability.

See:

* [Raspberry Pi logging and Mac synchronization](docs/data_logging.md)
* [current workflow](docs/next_steps.md)
* [project backlog and priorities](docs/improvements.md)
