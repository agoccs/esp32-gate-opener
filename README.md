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

The current development stage focuses on the measurement and analysis of
the existing 433.92 MHz RF signal.

The receiver-side measurement and analysis software has been implemented.

### Completed

- RXB60 receiver connected to ESP32
- Initial RF signal detection and identification
- RCSwitch-based protocol analysis
- Structured RF measurement collection
- Repeated measurements across multiple buttons and remote controls
- Python-based signal analysis
- Pulse timing statistics
- Signal frame visualization
- Binary signal pattern analysis

### Planned

- RF transmitter implementation
- Signal reproduction
- ESP32 Wi-Fi integration
- Remote control interface
- Cloud or bot-based gate control

## Repository structure

```text
esp32-gate-opener/
│
├── README.md
│
├── src/
│   └── receiver/
│       ├── README.md
│       ├── pulse_capture.ino
│       └── rc_switch_signal_test.ino
│
└── analysis/
    ├── README.md
    └── signal_analysis.py
