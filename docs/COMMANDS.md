# Operational Commands

Run repository commands from the project root unless a section explicitly says otherwise.

## Git state

```sh
git status
git branch
git log --oneline --decorate -5
```

## Firmware build

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Convenience script:

```sh
./scripts/compile.sh
```

Do not flash automatically unless explicitly intended.

Optional flashing script:

```sh
./scripts/compile_flash.sh
```

## Automated tests

```sh
cmake -S "unit test" -B "unit test/build"
cmake --build "unit test/build"
ctest --test-dir "unit test/build" --output-on-failure
python3 -m unittest discover -s "unit test" -p "test_*.py"
```

## Raspberry Pi access

Use the SSH configuration host, not a hard-coded IP address:

```sh
ssh raspi
```

## Data synchronization

Preview:

```sh
./scripts/sync_data.sh --dry-run
```

Pull Raspberry Pi data to the local repository:

```sh
./scripts/sync_data.sh
```

The synchronization direction is Raspberry Pi -> Mac and is non-destructive.

## Logger service

On Raspberry Pi:

```sh
sudo systemctl status vedirect-logger.service --no-pager
sudo systemctl start vedirect-logger.service
sudo systemctl stop vedirect-logger.service
sudo systemctl restart vedirect-logger.service
journalctl -u vedirect-logger.service --no-pager
```

Do not run a second manual logger while systemd owns the Pico serial device.

Install/update the version-controlled service when required:

```sh
sudo install -m 644 systemd/vedirect-logger.service /etc/systemd/system/
sudo systemctl daemon-reload
```

## Dashboard service

Install/update on Raspberry Pi:

```sh
sudo install -m 644 systemd/vedirect-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now vedirect-dashboard.service
sudo systemctl status vedirect-dashboard.service --no-pager
```

Administration:

```sh
sudo systemctl restart vedirect-dashboard.service
sudo systemctl stop vedirect-dashboard.service
journalctl -u vedirect-dashboard.service --no-pager
```

Trusted-LAN URL:

```text
http://raspi:8000
```

Do not expose port 8000 to the public internet in the current unauthenticated and unencrypted configuration.

## Data locations

Raspberry Pi:

```text
/home/fori/pico_cpp/data/raw/
/home/fori/pico_cpp/data/processed/
```

Mac/local repository:

```text
data/raw/
data/processed/
```

Processed daily file format:

```text
data/processed/vedirect_YYYY-MM-DD.csv
```

Raw daily file format:

```text
data/raw/vedirect_YYYY-MM-DD.log
```
