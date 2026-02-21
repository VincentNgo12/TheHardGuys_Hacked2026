# Smart Health Wristwatch — Firmware & Hardware Repository
## Overview

This repository contains the firmware and hardware design files for our ESP32-based low-power health-tracking wristwatch.
The system integrates:

Custom PCB featuring ESP32, MAX30102, ENS160 + AHT21, MPU-6500, and TFT LCD

Firmware written in the Arduino framework, but structured like a real embedded project

Cooperative FreeRTOS task design for sensor polling, UI updates, Bluetooth, and power management

Modular driver architecture so each sensor is isolated in its own component

## The goal:
A reliable, power-efficient, hackathon-ready prototype with clean architecture and scalable code structure.