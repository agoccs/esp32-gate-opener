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
```

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

## Data security

Raw RF measurements capable of reproducing a specific gate-control signal
are intentionally excluded from the public repository.

The repository contains the measurement and analysis methodology without
publishing reproducible gate-control data.

### RF transmitter

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

## Future development

The next development stages are:

1. Wi-Fi connectivity
2. Remote control interface
3. Cloud or bot-based gate control
