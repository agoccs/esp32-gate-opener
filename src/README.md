# Firmware overview

This folder contains the ESP32 firmware of the project, one folder per development stage. If you are new here, read them in this order:

1. [`receiver/`](receiver/README.md) – measures and records the 433.92 MHz signal of the existing remote control (RXB60 receiver). The recorded data is processed with the Python tools in [`../analysis`](../analysis/README.md).
2. [`transmitter/`](transmitter/README.md) – reproduces the identified signal with an RF433-JTX transmitter module (standalone test, no network).
3. [`esp32_gate/`](esp32_gate/README.md) – the Wi-Fi / Firebase firmware: connects to the network and reports its online status to the web control panel in [`../web`](../web/README.md). Command handling and the RF transmitter code will be merged into this one.
