# Arduino Multifunction Project

A small Arduino UNO project built to experiment with sensors, actuators,
user interfaces, and infrared remote control.

## Features

The project combines several peripherals in a single menu-based system:

-   **16x2 LCD** for the user interface
-   **Joystick** for menu navigation and control
-   **IR remote control** as an alternative input method
-   **Servo motor** with adjustable position
-   **DHT11** temperature and humidity sensor
-   **HC-SR04** ultrasonic distance sensor
-   **Active buzzer** and **LED** for alarms
-   **Physical BACK button** to return to the main menu
-   **LCD editor/game** with a movable cursor and an O/X grid

## Operating Modes

### 1. Servo

The servo angle can be controlled using the joystick or the IR remote.

### 2. Proximity

The HC-SR04 measures the distance to an object.\
The alarm threshold can be adjusted, and the LED and active buzzer are
activated when an object is detected within the configured distance.

A small software filter is used to reject isolated spurious ultrasonic
measurements.

### 3. Temperature & Humidity

The DHT11 measures temperature and relative humidity.\
The temperature alarm threshold can be adjusted by the user. When the
measured temperature reaches or exceeds the threshold, the LED and
buzzer are activated.

### 4. LCD Editor

The cursor can be moved across the 16x2 display and each cell can be
toggled between `O` and `X`.

When all 32 cells become `X`, the system displays:

> BRAVA ULISSA !! :)

and activates the LED and a short buzzer victory sequence.

## IR Remote Controls

From the main menu:

  Button    Function
  --------- ---------------------------
  `1`       Servo mode
  `2`       Proximity mode
  `3`       Temperature/Humidity mode
  `4`       LCD Editor
  `POWER`   Return to main menu

Within the modes, the arrow buttons control the relevant parameter or
cursor. `PLAY/PAUSE` toggles `O` / `X` in the LCD editor.

## Arduino UNO Pin Map

  Arduino Pin   Connection
  ------------- --------------------------------
  D2            DHT11 DATA
  D3            HC-SR04 TRIG
  D4            LCD D7
  D5            LCD D6
  D6            LCD D5
  D7            LCD D4
  D8            LED (through 220 ohm resistor)
  D9            Active buzzer
  D10           HC-SR04 ECHO
  D11           LCD Enable
  D12           LCD RS
  D13           Servo signal
  A0            Physical BACK button
  A1            Joystick X
  A2            Joystick Y
  A3            Joystick switch
  A4            IR receiver signal

All modules share a common **5 V** supply and **GND**.

## Libraries

The sketch uses:

-   `LiquidCrystal`
-   `DHT`
-   `Servo`
-   `IRremote`

## Repository Files

-   `Arduino_allarme_corretto.cpp` --- main Arduino source code
-   `Report_cablaggio_Arduino_progetto_multifunzione.docx` --- detailed
    wiring report for rebuilding the prototype

## Notes

The buzzer used in the final version is an **active buzzer** and is
driven with `digitalWrite()` rather than `tone()`. This avoids the timer
conflict encountered between `tone()` and IR reception.

The wiring report should be kept together with photographs of the
physical breadboard, since the source code identifies electrical
connections but not the exact physical breadboard rows used.
