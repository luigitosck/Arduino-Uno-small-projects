# Arduino MQ-2 Gas Monitor

A simple Arduino UNO project for experimenting with an **MQ-2 gas
sensor**, automatic warm-up/calibration, visual status indication, and
an audible alarm.

> **Important:** This is an educational/hobby project and is **not a
> certified gas safety detector**. Do not rely on it as the only
> protection against gas leaks.

## Features

-   MQ-2 analog gas sensing
-   10-minute sensor warm-up
-   30-second automatic baseline calibration
-   Automatically calculated experimental threshold (`baseline + 150`)
-   5-second persistence filter before triggering the alarm
-   Green LED for system ready / normal condition
-   Red LED for alarm
-   Passive buzzer alarm
-   Serial Monitor output for debugging

## Components

-   Arduino UNO
-   MQ-2 gas sensor module
-   Green LED
-   Red LED
-   2x 220 ohm resistors
-   Passive buzzer
-   Breadboard
-   Jumper wires

## Wiring

  Device             Connection
  ------------------ -------------------------------------------
  MQ-2 VCC           Arduino 5V
  MQ-2 GND           Arduino GND
  MQ-2 AO            Arduino A0
  MQ-2 DO            Not connected
  Green LED          D7 -\> 220 ohm -\> anode; cathode -\> GND
  Red LED            D8 -\> 220 ohm -\> anode; cathode -\> GND
  Passive buzzer +   D9
  Passive buzzer -   GND

## Operation

### 1. Warm-up

After power-up, the MQ-2 warms up for **10 minutes**. Both LEDs and the
buzzer remain off.

### 2. Calibration

Arduino samples the sensor for **30 seconds** and calculates the average
reading:

``` text
baseline = average of calibration samples
threshold = baseline + 150
```

Example:

``` text
Baseline = 330
Threshold = 480
```

Calibration should be performed in normal ambient air.

### 3. Monitoring

After calibration, the **green LED turns on** to indicate that
monitoring is active.

If the sensor stays below the threshold:

``` text
GREEN LED = ON
RED LED   = OFF
BUZZER    = OFF
```

### 4. Persistence filter

A short spike does not immediately trigger the alarm. The reading must
remain at or above the threshold for **5 consecutive seconds**. If it
drops below the threshold before that, the timer resets.

### 5. Alarm

After 5 seconds continuously above threshold:

``` text
GREEN LED = OFF
RED LED   = ON
BUZZER    = ON
```

The passive buzzer is driven with `tone()` at **1000 Hz**.

## Serial Monitor

Use **9600 baud**.

Typical output:

``` text
WARM-UP | MQ-2 = 526
WARM-UP | MQ-2 = 481

CALIBRAZIONE | MQ-2 = 335
CALIBRAZIONE | MQ-2 = 329

CALIBRAZIONE COMPLETATA
Baseline = 330
Soglia = 480
Sistema pronto!

MQ-2 = 342 | Baseline = 330 | Soglia = 480
```

## State Flow

``` text
POWER ON
   |
   v
10 min WARM-UP
   |
   v
30 s CALIBRATION
   |
   v
Calculate baseline
   |
   v
Threshold = baseline + 150
   |
   v
GREEN LED ON
Monitoring active
   |
   v
Reading >= threshold?
   |
   +-- No --> Continue monitoring
   |
  Yes
   |
   v
Start 5 s timer
   |
   v
Still above threshold for 5 s?
   |
   +-- No --> Reset timer
   |
  Yes
   |
   v
RED LED + BUZZER
```

## Notes

The MQ-2 is a heated semiconductor gas sensor, so its analog output can
change significantly after power-up. The Arduino ADC reading (`0-1023`)
is **not a universal ppm value or certified safety threshold**.

The sensor can also react to smoke and other combustible vapors, so
false positives are possible.

## Safety Notice

This project is intended for **learning, prototyping, and
experimentation only**. It must not replace a certified domestic gas
detector or other required safety equipment. Do not intentionally create
dangerous gas concentrations to test the circuit.

## Possible Future Improvements

-   16x2 LCD showing sensor value, baseline, and threshold
-   Warm-up/calibration status indication
-   Non-blocking alarm siren pattern
-   Data logging
-   Improved calibration algorithm
-   Enclosure and standalone 5 V power supply
