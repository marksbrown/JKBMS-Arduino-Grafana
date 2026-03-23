# JKBMS-Arduino-Grafana

Reads data from a JK BMS over UART using an Arduino Uno R4 WiFi and posts it to a MariaDB database via the MaxScale REST API, for display in Grafana.

Tested with a 4-cell LiFePO4 pack (EVE A-grade, 3.2 V nominal).

## Architecture

```
JK BMS ──UART──► Arduino Uno R4 WiFi ──HTTPS──► MaxScale API ──► MariaDB ──► Grafana
```

The Arduino polls the BMS every 5 seconds and inserts a row into the `bms` table on each successful read.

## Requirements

- Arduino Uno R4 WiFi
- JK BMS with UART port
- MariaDB with MaxScale REST API
- Grafana (optional, for dashboards)

## Setup

### 1. Credentials

Copy the secrets template and fill in your values:

```bash
cp bms/arduino_secrets.h.example bms/arduino_secrets.h
```

Edit `bms/arduino_secrets.h`:

| Define | Description |
|---|---|
| `SECRET_SSID` | WiFi network name |
| `SECRET_PASS` | WiFi password |
| `SECRET_HTTPUSER` | MaxScale API username |
| `SECRET_HTTPAUTH` | MaxScale API password |

### 2. Database

Create the table using the provided schema:

```bash
mysql -u root -p < db/bms.sql
```

### 3. Build

```bash
./build.sh
```

The script installs `arduino-cli` if missing, then installs the required board package, libraries, and compiles the sketch. Firmware is written to `build/output/`.

### 4. Upload

```bash
./build.sh --upload /dev/ttyACM0
```

## JKBMSInterface library

The `JKBMSInterface/` directory is a local fork of [chrissank/JKBMSInterface](https://github.com/chrissank/JKBMSInterface) with the following fixes applied:

- **Checksum validation** — incoming frames are now verified against the protocol checksum before parsing; corrupt frames (e.g. from electrical noise) are silently discarded
- **Current mask** — charge current above ~20 A was being truncated due to an 11-bit mask (`0x07FF`); corrected to 15-bit (`0x7FFF`) per the protocol spec
- **Frame detection** — replaced fragile end-pattern heuristic with STX + length-field framing, ensuring the complete frame (including checksum bytes) is always available before parsing

## Grafana

Import `grafana/dashboard.json` into your Grafana instance and point it at the MariaDB datasource.
