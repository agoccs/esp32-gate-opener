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

The full remote-control path is implemented and ready for testing at the gates:

- the ESP32 connects to Wi-Fi, sets its clock from NTP and signs in to Firebase over certificate-validated HTTPS;
- every 10 seconds it reports a heartbeat (`device/lastSeen`) and the Wi-Fi signal strength (`device/rssi`);
- the web control panel (Firebase Hosting) requires a login, shows whether the ESP32 is online and has three gate buttons;
- a button press creates a command in the Realtime Database; the ESP32 polls for new commands, checks them, transmits the RF code once and reports the result back to the page;
- database rules allow only listed users to send commands and only the ESP32 account to handle them.

### Completed

- RXB60 receiver connected to ESP32, RF signal detection and protocol identification
- Python-based signal analysis (timing statistics, frame and bit-pattern visualization)
- Standalone RF433-JTX transmitter test with RCSwitch
- ESP32 Wi-Fi connection and authenticated Firebase heartbeat, with TLS certificate validation (Google root certificates, NTP time)
- Firebase-hosted web UI with login and ESP32 online/offline indicator
- Command path: web buttons → database → ESP32 → RF transmission → status back to the page
- Safety logic: at-most-once execution, 20 s expiry, per-gate cooldown, rate limit, no replay after restart
- Database security rules with an allow-list of users and validated command fields
- Software watchdog (restart after 10 minutes without a successful heartbeat)

### Planned

- Field test at the gates (range, antenna position, Wi-Fi signal in the garage)
- Final documentation with wiring diagram and photos

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
│   ├── esp32_gate/          # Final ESP32 firmware (Wi-Fi, Firebase, commands, RF)
│   │   ├── README.md
│   │   ├── esp32_gate.ino
│   │   ├── google_roots.h
│   │   ├── secrets.example.h
│   │   └── rf_codes.example.h
│   ├── receiver/            # RF measurement firmware
│   └── transmitter/         # Standalone RF transmitter test
│
└── analysis/                # Python signal analysis
```

## System architecture

```
 Phone / browser (web/)                              ESP32 (src/esp32_gate)
      │                                                     │
      │ email + password login                              │ device account login (REST)
      ▼                                                     ▼
 Firebase Authentication ──── ID token ────┬──── ID token ──┘
                                           ▼
                            Firebase Realtime Database
   button press ──► commands/<id>  status: pending  ◄── poll every 2 s
                                    status: taken   ◄── written BEFORE transmitting
                                    status: done    ◄── after the RF code was sent
   page follows the status ◄───────
   page shows Online/Offline ◄───── device/lastSeen, device/rssi ◄── every 10 s
                                                                      │
                                                                      ▼
                                                     RF433-JTX ──433.92 MHz──► gate
```

Each command is executed at most once: the ESP32 marks it `taken` before transmitting, so a restart in between can lose a command but never repeat it. Commands older than 20 seconds, or sent before the ESP32 started, expire without transmission.

## Configuration and secrets

Files containing credentials are **not** in the repository. Each has an `example` template that you copy and fill in locally:

| Template (in Git) | Local copy (ignored by Git) | Contains |
|---|---|---|
| `web/firebase-config.example.js` | `web/firebase-config.local.js` | Firebase web app configuration |
| `src/esp32_gate/secrets.example.h` | `src/esp32_gate/secrets.h` | Wi-Fi credentials, Firebase API key and database URL, ESP32 device account |
| `src/esp32_gate/rf_codes.example.h` | `src/esp32_gate/rf_codes.h` | RF protocol, timing and the three gate codes |

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
- ESP32 Arduino core (`WiFi`, `WiFiClientSecure`, `HTTPClient`) and the ArduinoJson library

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

The final firmware in [`src/esp32_gate`](src/esp32_gate/README.md) drives the same transmitter; its RF settings come from the private `rf_codes.h`.

## Data security

Raw RF measurements capable of reproducing a specific gate-control signal
are intentionally excluded from the public repository.

The repository contains the measurement and analysis methodology without
publishing reproducible gate-control data.

Raw RF measurements and working RF codes capable of reproducing a specific gate-control signal are never published. Credentials (Wi-Fi, Firebase, RF parameters) live only in local files that are listed in `.gitignore`.

## Future development

1. Gate position feedback (e.g. a magnetic switch), so the page shows whether a gate is open
2. Authenticated remote firmware update
3. User management on the page instead of the Firebase console
4. Optional: other control interfaces (e.g. bot-based control)
