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

## Current stage

The current development stage focuses on measuring and characterizing
the 433.92 MHz RF signal using an RXB60 receiver connected to an ESP32.

Two firmware programs are currently used for this stage.

## Repository structure

```text
src/
└── receiver/
    ├── rc_switch_signal_test/
    │   └── rc_switch_signal_test.ino
    │
    └── pulse_capture/
        └── pulse_capture.ino
