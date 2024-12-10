# EVE Display Demo for nRF5340-DK

This project demonstrates using a Bridgetek EVE display with the nRF5340-DK development kit.

## Hardware Setup

Connect the EVE display to the nRF5340-DK using the Arduino headers:

- SCK: P1.15
- MOSI: P1.14
- MISO: P1.13
- CS: P1.12
- PD: P1.10 (Power Down)
- INT: P1.11 (Interrupt)

## Software Requirements

- nRF Connect SDK v2.8.0
- Zephyr RTOS
- CMake
- Python 3.x

## Building

1. Open nRF Connect SDK command prompt (from Toolchain Manager)
2. Navigate to project directory:

