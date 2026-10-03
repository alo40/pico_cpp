Before making changes:

1. Read `AGENTS.md`.
2. Read the current documentation:
   - `docs/data_logging.md`
   - `docs/improvements.md`
   - `docs/next_steps.md`
3. Inspect the current `01_data_inspection.ipynb`.
4. Inspect the available synchronized CSV files under the project-relative:
   `data/processed/`

Scope:
- Modify the analysis notebook only.
- Do not change Pico firmware, Raspberry Pi logging, synchronization scripts, systemd files, or acquisition code.
- Do not implement multi-file/date-range analysis yet.
- Do not add SSH, SCP, SFTP, rsync, or Raspberry Pi access to the notebook.
- Do not modify documentation, datasets, scripts, tests, configuration, or any other repository file.
- Keep changes minimal. The notebook may already contain some or all requested behavior; verify the current implementation and change only what is still necessary.

Task:

Bring `analysis/notebooks/01_data_inspection.ipynb` into compliance with the current synchronized-local-data workflow described in the documentation.

Required behavior:

1. Load data only from the project-relative `data/processed/` directory. Resolve the repository root robustly so the notebook works when launched from the repository root or from its notebook directory.
2. Analyze exactly one explicitly selected CSV file. Provide a simple, visible configuration value such as `DATA_FILE` containing a filename relative to `data/processed/`.
3. Do not silently select a file by modification time. If no file is selected, list the available CSV filenames in a deterministic order and stop with a clear instruction. Reject absolute paths, traversal outside `data/processed/`, missing files, and non-CSV selections.
4. Clearly display the selected filename or resolved path before analysis so the input is inspectable in the saved notebook output.
5. Validate the required schema:
   `timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w`.
   Fail clearly for missing columns, an empty dataset, invalid or missing timestamps, missing/non-numeric measurement values, or non-finite numeric values.
6. Parse `timestamp` as timezone-aware data while preserving the logger-host local offset. Do not implicitly convert it to UTC. Notebook prose must state that this is the Raspberry Pi logger's local receive/processing timestamp for Raspberry Pi datasets, not an MPPT or Pico hardware timestamp.
7. Keep the existing useful integrity analysis and four basic time-series plots. Ensure they operate only on the explicitly selected dataframe and report enough context to identify the selected dataset, row count, recording interval, and timezone/offset.
8. Keep units and meanings accurate: battery and panel voltage are stored in mV, battery current in mA, panel power in W, and positive battery current means charging while negative means discharging.
9. Do not claim that unmeasured quantities such as irradiance, temperature, or accurate battery state of charge are present or directly inferred.
10. Keep all acquisition, synchronization, and network operations outside the notebook.

Dataset choice:

- Inspect the synchronized CSV files and choose one explicit single-file dataset suitable for the saved verification run, preferably one containing meaningful daylight PV variation.
- Base that choice on file contents, not filesystem modification time.
- Do not combine files and do not alter source CSV data.

Verification:

1. Restart the notebook state and execute every cell once, in order, from top to bottom using the repository's available Jupyter environment.
2. Save the successful outputs in `analysis/notebooks/01_data_inspection.ipynb` with sequential execution counts and no stale or out-of-order results.
3. Confirm all integrity summaries and all four plots are produced without errors for the selected synchronized dataset.
4. Inspect the final git diff and verify that `analysis/notebooks/01_data_inspection.ipynb` is the only modified file from this task. Do not revert or overwrite pre-existing unrelated changes.

Completion report:

- Summarize the notebook changes.
- Name the selected CSV and explain briefly why it was chosen.
- Report the clean-execution result and any important integrity findings from the dataset.
- Explicitly state whether any requested behavior was already present and therefore left unchanged.
