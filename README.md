# esp2400

A digitalizing power supply control board woking with modified ZXD2400 (50V 50A) modoule.

This project is a control board mainly using ESP32 as core MPU, with ST7789V display,
including software source codes, hardware design, and other parts design,
to control a simply modified ZXD2400 power supply module.

In fact, this project is suitable for modifying other power supply module too.

its main functions include:
. 0-50.00V and 0-50.00A output with internal 14bits resolution
. Multi-points calibration of voltage and current.
. Two EC11 rotaries to control voltage and current separately
. Multi quick-pick of voltage and current by single pressing button
. Statistics by seconds, minutes, hours
. WIFI STA and AP support with a simple web view for remote control
. Command line support with USB-Serial interface (also firmware updating interface)
. Temperature controlled FAN output using PID algorithm
. A discharging circuit to decrease dormancy of switching power supply at low-voltage outputing
. Rate of rising and falling of voltage and current can be controlled
. Chinese and English bundles support (but in fact, original ZXD2400 with its PFC circiut can be used in AC220V zones)


