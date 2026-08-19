# Next Steps: VE.Direct Data Logging to macOS

- [ ] Add USB `printf()` output to the Pico firmware so one `DATA,...` record is sent after each complete VE.Direct block (`Checksum`).
- [ ] Compile and flash the updated Pico firmware.
- [ ] On macOS, find the Pico USB serial device with:
  `ls /dev/cu.usbmodem*`
- [ ] Open the Pico serial stream with `screen`.
- [ ] Verify that one `DATA,...` record arrives approximately once per second.
- [ ] Create a macOS logger script that reads the Pico USB serial stream.
- [ ] Add a macOS timestamp to every received record.
- [ ] Save the records continuously into a `.csv` file.
- [ ] Verify the CSV structure and recorded values.
- [ ] Expand the Pico parser/output to include additional VE.Direct fields.
- [ ] Implement proper VE.Direct checksum validation before storing/sending each block.
- [ ] Optionally add live plotting or later analysis of the CSV data on the Mac.