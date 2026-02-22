# Smart Health Wristwatch — Firmware & Hardware Repository
## NEST — Nursing Environment & Safety Tracker
### Smart Health Wristwatch (Firmware + Hardware)

NEST is a **low-power ESP32 wristwatch prototype** designed for older adults or people that does not have the ability to take care of themselves.  
It helps users feel **safer, more comfortable, and more independent** by making key wellness and environment signals easy to check at a glance—without complicated menus or medical jargon.

**Our goal:** a reliable, power-efficient, hackathon-ready prototype with clean architecture and scalable code structure.

---

## Overview
This repository contains the **firmware and hardware design files** for our ESP32-based health + comfort wristwatch prototype.

**The system integrates:**
- **ESP32** (main MCU, low-power capable)
- **MAX30102** (heart rate / pulse sensing)
- **ENS160 + AHT21** (air quality + temperature + humidity)
- **MPU-6500** (motion sensing)
- **TFT LCD** (watch UI)

Firmware is built using the **Arduino framework**, but structured like a real embedded project:
- **Modular driver architecture** (each sensor isolated)
- **Cooperative FreeRTOS-style task design** (sensor polling, UI updates, Bluetooth, power management)

---

## High-level idea
NEST provides two kinds of awareness:

### 1) Health awareness
- Quick, on-demand **heart-rate checks** (simple “tap to check” experience)

### 2) Comfort awareness
- Indoor **temperature + humidity**
- **Air quality trends** (eCO2 / TVOC-style indicators)

**Design philosophy:** reassurance without overwhelm  
- big text, minimal taps  
- friendly labels + icons  
- clear prompts like: *“All good”*, *“Room feels stuffy”*, *“Take a break”*  
- optional gentle alerts (vibration) instead of loud alarms  

---

## System architecture (high-level)
NEST is organized as a clean pipeline so the UI stays responsive while keeping power low:

**Sensors → Drivers → Tasks/Scheduler → UI + Alerts → (Optional) Bluetooth**

A simple mental model:

[ MAX30102 ]   [ ENS160 + AHT21 ]   [ MPU-6500 ]
      |               |                |
      +------->  Driver Layer (modular) <------+
                      |
                Task / Timing Layer
      (polling, smoothing, thresholds, scheduling)
                      |
        +-------------+-------------+
        |                           |
   TFT Watch UI                 Alerts / BT
 (big text, icons)        (vibration + summaries)


---

## Core concepts
### 1) Power-first behavior
NEST is designed around quick “check-ins”:
- measure briefly
- update the screen
- return to low-power mode whenever possible

A simple way to think about average power is:
\[
P_{\text{avg}} \approx \frac{P_{\text{active}}\,t_{\text{active}} + P_{\text{sleep}}\,t_{\text{sleep}}}{t_{\text{active}} + t_{\text{sleep}}}
\]
So the main strategy is keeping **\(t_{\text{active}}\)** short and spending most time sleeping.

### 2) Modular drivers
Each sensor is isolated in its own component so the project stays:
- easier to debug
- easier to test
- easier to expand later

### 3) Cooperative task design
Even when using Arduino, the project is structured to behave like a real embedded system:
- sensor reading doesn’t block UI updates  
- timing is predictable  
- Bluetooth/power management can be added cleanly  

---

## Repo structure
High-level folders (as included in this repository):
- `firmware/` — ESP32 firmware (Arduino-based, modular, task-oriented)
- `hardware/` — hardware design files (PCB / wiring / docs)
- `README.md` — this document

---

## Quick start (edit to match your setup)
1. Open `firmware/` in **Arduino IDE** (or PlatformIO if you migrate later).
2. Install required libraries for the display + sensors.
3. Select your ESP32 board, connect over USB, then **flash**.
4. Verify: UI boots, sensors respond, and readings update.

---

## Credits
**Team: The Hard Guys**  
- Vincent Ngo: damduchu@ualberta.ca
- Luu Gregory Nguyen: luugrego@ualberta.ca
- Jishnu Khanna: jkhanna@ualberta.ca
- Finhas Tadesse: finhas@ualberta.ca
- Joao Pedro Assis Leitao: assislei@ualberta.ca

Special thanks:
- Hackathon organizers + mentors
- Open-source libraries and community resources

---

## Disclaimer
NEST is a **hackathon prototype** intended for wellness/comfort awareness and is **not a medical device**. 


