# Next Steps: VE.Direct Data Logging to macOS

- [x] Implement proper VE.Direct checksum validation over each complete received block.
- [x] Store incoming fields in a temporary measurement snapshot.
- [x] Publish the snapshot only when its checksum is valid and all required fields are present.
- [x] Show parser-health diagnostics on OLED 3.
- [x] Build, test, and flash the parser-health firmware successfully.
- [ ] Track a monotonically increasing sequence number for each published snapshot.
- [ ] Define the initial CSV columns as:
  `sequence,battery_mv,panel_mv,battery_ma,panel_w`
- [ ] Add USB `printf()` output to the Pico firmware, including one CSV header after startup.
- [ ] Send exactly one CSV data row for each published, validated snapshot.
- [ ] Compile and flash the updated data-logging firmware.
- [ ] On macOS, find the Pico USB serial device with:
  `ls /dev/cu.usbmodem*`
- [ ] Open the Pico serial stream with `screen`.
- [ ] Verify that one CSV row arrives for each valid VE.Direct block (normally approximately once per second).
- [ ] Confirm that incomplete or checksum-invalid blocks never produce CSV rows.
- [ ] Create a macOS logger script that reads the Pico USB serial stream.
- [ ] Add a macOS timestamp to every received row while retaining the Pico sequence number.
- [ ] Save the records continuously into a `.csv` file.
- [ ] Verify the CSV structure and recorded values.
- [ ] Expand the Pico parser/output to include additional VE.Direct fields.
- [ ] Optionally add live plotting or later analysis of the CSV data on the Mac.
