# Access Control System

ESP32-based RFID access control system with a Node.js/Express backend and MongoDB storage.  
Supports card validation, logging, and real-time authorization over HTTP.

<p align="center">
  <!-- Update the path once you upload your photo -->
  <img src="./docs/images/device-front.jpg" width="450" alt="Access Control System Device">
</p>

---

## Overview

This project is a complete access control system built around an ESP32 microcontroller and an RFID reader.  
The ESP32 reads RFID cards, sends authorization requests to a backend server over HTTP, and then unlocks or denies access based on the server response.

The system includes:

- **Embedded firmware** running on an ESP32
- **Backend server** built with Node.js/Express and MongoDB
- **3D-printed enclosure** for the final physical product

---

## Features

- ESP32-based controller with RFID reader
- HTTP communication between device and backend
- Node.js/Express server that validates cards and logs access attempts
- MongoDB database for storing authorized cards and event logs
- Real-time authorization (allow/deny) responses to the device
- Custom 3D-printed casing for the final hardware

---

## System Architecture

```text
[ RFID Card ] --> [ ESP32 + RFID Reader ] --> HTTP Request --> [ Node.js/Express Backend ] --> [ MongoDB ]
                                          <-- HTTP Response (ALLOW / DENY) <--
