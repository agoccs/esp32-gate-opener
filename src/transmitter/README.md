# RF Transmitter

This directory contains the ESP32 firmware used to transmit 433.92 MHz RF signals through an RF433-JTX transmitter module.

The transmitter is controlled by an ESP32 and uses the RCSwitch library to generate the RF signal.

## Programs

### `transmitter.ino`

The transmitter program configures the RF433-JTX module and sends a configured RF code using the RCSwitch library.

The program provides the following functionality:

- initializes the transmitter output;
- configures the RF protocol;
- sets the number of transmission repetitions;
- configures the RF pulse duration;
- transmits the configured code once during startup.

The public version does not contain a working device-specific RF code or pulse duration. These parameters must be configured locally based on measurements of the target remote control.

## Hardware

Current transmitter setup:

- ESP32
- RF433-JTX 433 MHz ASK/OOK transmitter module
- 433 MHz antenna

The transmitter module is connected to the ESP32 as follows:

| Transmitter pin | ESP32 connection |
|---|---|
| DATA | GPIO 25 |
| VCC | 5V / VIN |
| GND | GND |

An external antenna is connected to the transmitter module.

## Configuration

Before transmitting, configure the following parameters in `transmitter.ino`:

- `RF_CODE`: RF code identified from your own measurements.
- `RF_BIT_LENGTH`: number of bits in the transmitted code.
- `RF_PULSE_US`: RF pulse duration in microseconds, determined experimentally.

The default values prevent transmission until the RF code and pulse duration have been configured.

Start by testing the pulse duration determined from your measurements. If transmission is unreliable, test several nearby values and document the results.

**Note:** `RF_PULSE_US` specifies the duration of the RF protocol pulses in microseconds. It is not the same as the Arduino `delay()` function, which uses milliseconds.

## Transmission workflow

```text
Configured RF parameters
          ↓
      ESP32
          ↓
  RCSwitch library
          ↓
   RF433-JTX module
          ↓
    433 MHz antenna
          ↓
    Target receiver
```

## Safety

Only test with devices you own or are authorized to control.

Do not publish working access codes or device-specific RF parameters in a public repository. The public example uses placeholder values and must be configured locally before transmission.

