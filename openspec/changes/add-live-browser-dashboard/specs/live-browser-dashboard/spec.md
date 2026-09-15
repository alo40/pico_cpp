## Purpose

Provide a read-only, trusted-LAN view of current solar measurements and the
complete local calendar day's measurement trend without altering acquisition.

## ADDED Requirements

### Requirement: Trusted-LAN dashboard access
The system SHALL provide an unauthenticated, read-only HTTP dashboard reachable
by devices on the trusted local network. It SHALL not modify acquisition,
logger, or processed data.

#### Scenario: Dashboard opens on the LAN
- **WHEN** a device on the trusted local network requests the dashboard URL
- **THEN** it receives the dashboard interface without requiring credentials

#### Scenario: Dashboard is unavailable to its data source
- **WHEN** the current processed data file cannot be read
- **THEN** the dashboard SHALL report that live data is unavailable without
  altering any data file

### Requirement: Current measurement summary
The dashboard SHALL display the latest valid battery voltage, panel voltage,
battery current, panel power, and sample timestamp from the current local
calendar day's processed CSV.

#### Scenario: Latest sample is displayed
- **WHEN** the current daily CSV contains valid measurements
- **THEN** the dashboard SHALL display values from its latest valid measurement
  and its timestamp

#### Scenario: No measurement exists today
- **WHEN** the current daily CSV is absent or has no valid measurement
- **THEN** the dashboard SHALL display that no live measurement is available

### Requirement: Today-so-far graphs
The dashboard SHALL graph battery voltage, panel voltage, battery current, and
panel power across the complete current local calendar day from midnight through
the latest valid sample. Visual point reduction SHALL not change the represented
time range.

#### Scenario: Current-day history is graphed
- **WHEN** valid measurements span part of the current local day
- **THEN** each graph SHALL represent the interval from that day's midnight to
  the timestamp of the latest valid sample

#### Scenario: Large current-day history is displayed
- **WHEN** the current daily CSV contains more samples than can be drawn
  individually at the available display width
- **THEN** the dashboard SHALL preserve the full current-day time range while
  reducing displayed point density

### Requirement: Live updates and daily rollover
The dashboard SHALL incorporate valid rows appended to the current daily CSV
without requiring a page reload. It SHALL transition to the new local day's CSV
when the date changes.

#### Scenario: Logger appends a valid row
- **WHEN** a new valid row is appended to the current daily CSV
- **THEN** the dashboard SHALL update its latest values and current-day graphs

#### Scenario: Local date changes
- **WHEN** the dashboard detects a new local calendar day
- **THEN** it SHALL display the new day's data and no longer present the prior
  day's measurements as current data

### Requirement: Data freshness indication
The dashboard SHALL display the age of its latest valid sample and visibly mark
the data as stale when no new valid sample has arrived for two minutes.

#### Scenario: Recent data is healthy
- **WHEN** the latest valid sample is less than two minutes old
- **THEN** the dashboard SHALL show its age without a stale warning

#### Scenario: Data becomes stale
- **WHEN** the latest valid sample is at least two minutes old
- **THEN** the dashboard SHALL visibly indicate that the displayed data is stale
