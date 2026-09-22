# RF Receiver

This directory contains the ESP32 firmware used for the measurement and
characterization of the 433.92 MHz RF signal.

The receiver hardware is based on an RXB60 433.92 MHz RF receiver module
connected to an ESP32.

## Programs

### `rc_switch_signal_test.ino`

Initial test program used to verify the communication between the RXB60
receiver and the ESP32.

The program uses the RCSwitch library to identify received signals and
display:

- decoded signal value
- bit length
- detected protocol
- delay
- raw pulse timing data

This program was used during the initial investigation of the received
433.92 MHz signal.

### `pulse_capture.ino`

Measurement program used to collect a structured dataset for further
analysis.

The program performs repeated measurements for multiple buttons and
remote controls. Each measurement is written to the serial output in a
CSV-compatible format containing:

- button identifier
- remote control identifier
- sample number
- decoded signal value
- bit length
- protocol
- delay
- raw pulse timing data

The collected data is subsequently processed using the Python analysis
tools located in the `analysis` directory.

## Hardware

Current receiver setup:

- ESP32
- RXB60 433.92 MHz receiver
- 433.92 MHz remote control

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
    Serial measurement data
          ↓
    Python data analysis
