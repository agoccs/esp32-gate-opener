# ESP32 Gate Opener

ESP32-based smart gate control system with 433.92 MHz RF signal analysis,
RF transmission, Wi-Fi connectivity and remote control.

## Project overview

The project aims to develop an ESP32-based system for remotely controlling
a gate using the existing 433.92 MHz RF interface.

The development process is divided into several stages:

1. Measurement and identification of the existing RF signal.
2. Detailed analysis of the signal timing and protocol.
3. Reproduction of the RF transmission.
4. Integration of Wi-Fi connectivity.
5. Implementation of remote control through a network or cloud-based interface.

## Current status

The Wi-Fi / cloud part is working end to end:

- the ESP32 connects to Wi-Fi and signs in to Firebase Authentication with its own device account;
- every 10 seconds it writes a server-side timestamp to the Firebase Realtime Database (`device/lastSeen`);
- the web control panel (Firebase Hosting) requires a login and shows whether the ESP32 is online;
- database rules allow only logged-in users to read the status and only the ESP32 account to write it.

The RF side (measurement, analysis and a standalone transmitter test) was finished in the earlier stages. The two parts are not merged yet: the three web buttons are still disabled and the Wi-Fi firmware does not transmit RF signals yet.

### Completed

- RXB60 receiver connected to ESP32, RF signal detection and protocol identification
- Python-based signal analysis (timing statistics, frame and bit-pattern visualization)
- Standalone RF433-JTX transmitter test with RCSwitch
- ESP32 Wi-Fi connection and authenticated Firebase heartbeat
- Firebase-hosted web UI with login and ESP32 online/offline indicator
- Database security rules (read: logged-in users, heartbeat write: ESP32 account only)

### Planned

- Command path: web buttons → database → ESP32 (command validation, single execution)
- Merging the RF transmitter code into the Wi-Fi firmware
- Certificate validation for the ESP32's HTTPS connections
- Safe behavior after restart / power loss

## Repository structure

```
esp32-gate-opener/
│
├── README.md
├── .gitignore
├── firebase.json            # Firebase Hosting + database rules config
├── .firebaserc              # Firebase project alias
├── database.rules.json      # Realtime Database security rules
│
├── web/                     # Web control panel (Firebase Hosting)
│   ├── README.md
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   └── firebase-config.example.js
│
├── src/
│   ├── esp32_gate/          # ESP32 Wi-Fi + Firebase firmware
│   │   ├── README.md
│   │   ├── esp32_gate.ino
│   │   └── secrets.example.h
│   ├── receiver/            # RF measurement firmware
│   └── transmitter/         # Standalone RF transmitter test
│
└── analysis/                # Python signal analysis
```

## System architecture

```
 Browser (web/)                                   ESP32 (src/esp32_gate)
      │                                                  │
      │ 1. email + password login                        │ 1. device account login
      ▼                                                  ▼
 Firebase Authentication ◄───────────── (REST, HTTPS) ───┘
      │
      │ 2. ID token                                      2. ID token
      ▼                                                  ▼
 Firebase Realtime Database ◄── PUT device/lastSeen = server timestamp (every 10 s)
      │
      └──► onValue(device/lastSeen) → "Online" if younger than 45 s
```

The heartbeat is written as a server-side timestamp, so the ESP32 does not need an accurate clock. The web page compares it with the browser's time.

Planned extension: the buttons write a command into the database, the ESP32 reads it, transmits the RF code and acknowledges it.

## Configuration and secrets

Files containing credentials are **not** in the repository. Each has an `example` template that you copy and fill in locally:

| Template (in Git) | Local copy (ignored by Git) | Contains |
|---|---|---|
| `web/firebase-config.example.js` | `web/firebase-config.local.js` | Firebase web app configuration |
| `src/esp32_gate/secrets.example.h` | `src/esp32_gate/secrets.h` | Wi-Fi credentials, Firebase API key and database URL, ESP32 device account |

Setup instructions: [`web/README.md`](web/README.md) (Firebase project, users, rules, hosting) and [`src/esp32_gate/README.md`](src/esp32_gate/README.md) (firmware).

## Receiver

The `src/receiver` directory contains the ESP32 firmware used for
433.92 MHz RF signal measurement.

### `rc_switch_signal_test.ino`

Initial test program used to verify the RXB60 receiver and ESP32 interface.

The program uses the RCSwitch library to display:

- decoded signal value
- bit length
- detected protocol
- delay
- raw pulse timing data

### `pulse_capture.ino`

Structured measurement program used to collect repeated RF measurements.

The program records measurements for multiple buttons and remote controls
and outputs the results in a CSV-compatible format.

Each measurement contains:

- button identifier
- remote control identifier
- sample number
- decoded signal value
- bit length
- protocol
- delay
- raw pulse timing data

See [`src/receiver/README.md`](src/receiver/README.md) for more details.

## Signal analysis

The `analysis` directory contains the Python tools used to process the
measurement data collected by the ESP32.

The analysis currently includes:

- signal code identification
- delay analysis
- short and long pulse statistics
- pulse-pair validation
- binary signal decoding
- typical frame visualization
- bit-pattern visualization
- cleaned measurement data generation
- protocol summary generation

See [`analysis/README.md`](analysis/README.md) for more details.

## Hardware

Current receiver hardware:

- ESP32
- RXB60 433.92 MHz receiver
- 433.92 MHz remote controls

The RXB60 data output is connected to GPIO 34 of the ESP32.

## Measurement workflow

```text
433.92 MHz remote control
          ↓
        RXB60
          ↓
         ESP32
          ↓
    pulse_capture.ino
          ↓
    Measurement data
          ↓
  signal_analysis.py
          ↓
   Signal characterization
```
## Software

- Arduino IDE
- ESP32 Arduino framework
- RCSwitch library
- Python
- NumPy
- pandas
- Matplotlib
- Firebase (Authentication, Realtime Database, Hosting) and the Firebase CLI
- HTML / CSS / JavaScript (web control panel)
- ESP32 Arduino core (`WiFi`, `WiFiClientSecure`, `HTTPClient`; no extra libraries for the Wi-Fi firmware)

## RF transmitter

The RF transmitter is based on the RF433-JTX 433 MHz ASK/OOK module, controlled by an ESP32 using the RCSwitch library.

**Hardware connections**
- DATA → ESP32 GPIO25
- GND → ESP32 GND
- VCC → ESP32 5V/VIN
- External antenna connected to the transmitter module

### `transmitter.ino`
**Configuration**

The public example does not contain the original remote-control codes or device-specific timing values. These parameters must be determined experimentally for the target transmitter and receiver.

- `RF_CODE`: the code obtained from the user's own measurements.
- `RF_BIT_LENGTH`: the measured or decoded bit length.
- `RF_PULSE_US`: the pulse duration in microseconds.

Start by testing the measured pulse duration. If the transmission is unreliable, test several values around the measured duration and document the results.

The `delay()` function is used for software timing between operations; it is not the same as the RF protocol's pulse duration.

**Safety note:** Only test with devices you own or are authorized to control. Do not expose real access codes or working credentials in a public repository.

The Wi-Fi / Firebase firmware that will later use this transmitter is in [`src/esp32_gate`](src/esp32_gate/README.md).

## Data security

Raw RF measurements capable of reproducing a specific gate-control signal
are intentionally excluded from the public repository.

The repository contains the measurement and analysis methodology without
publishing reproducible gate-control data.

Raw RF measurements and working RF codes capable of reproducing a specific gate-control signal are never published. Credentials (Wi-Fi, Firebase, RF parameters) live only in local files that are listed in `.gitignore`.

## Future development

1. Command path from the web buttons to the ESP32
2. Merging RF transmission into the Wi-Fi firmware
3. Hardening: certificate validation, restricting control to named users
4. Optional: other control interfaces (e.g. bot-based control)
