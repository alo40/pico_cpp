## Purpose

Let trusted-LAN dashboard users select retained daily VE.Direct CSV files and
inspect each day's measurements without altering acquisition or stored data.

## ADDED Requirements

### Requirement: Available recorded days can be selected
The dashboard SHALL provide a selector containing `Today (live)` and every
available daily processed CSV date. `Today (live)` SHALL be selected when the
dashboard first opens, and recorded dates SHALL be presented newest first.

#### Scenario: Dashboard has retained daily files
- **WHEN** processed daily CSV files are available
- **THEN** the selector SHALL offer their dates in newest-first order alongside
  `Today (live)`

#### Scenario: No recorded day is available
- **WHEN** no processed daily CSV file is available
- **THEN** the selector SHALL still offer `Today (live)` and the dashboard SHALL
  retain its existing unavailable-data behavior

### Requirement: Selected historical day is displayed as a static view
The dashboard SHALL replace its displayed measurements and graphs with the
valid rows from a selected historical daily CSV. A historical selection SHALL
remain selected without live polling or automatic rollover until the user
selects `Today (live)`.

#### Scenario: User selects a recorded day
- **WHEN** the user selects a historical date
- **THEN** the dashboard SHALL display that date's valid measurements and graphs
  from midnight through its latest valid sample

#### Scenario: Historical data status is displayed
- **WHEN** a historical day has valid measurements
- **THEN** the dashboard SHALL show the selected date and its latest sample time
  without showing a stale-data warning

### Requirement: Returning to Today restores live updates safely
The dashboard SHALL replace its dataset when the selected day changes. Selecting
`Today (live)` SHALL restore the existing incremental updates and local-midnight
rollover behavior without using sequence state from a historical day.

#### Scenario: User returns from history to Today
- **WHEN** the user selects `Today (live)` after viewing a historical day
- **THEN** the dashboard SHALL load a complete current-day dataset before it
  resumes incremental updates

#### Scenario: Local date changes while Today is selected
- **WHEN** the local calendar day changes while `Today (live)` is selected
- **THEN** the dashboard SHALL replace the prior day's dataset with the new
  current-day dataset
