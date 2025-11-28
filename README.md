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
```

- The ESP32 reads the RFID UID and sends it to the backend.
- The backend checks the UID against the database and decides whether to allow or deny access.
- The decision is sent back to the ESP32, which can, for example, toggle a relay / LED / buzzer (depending on how you wired it).

---

## Hardware

Main components:

- ESP32 development board
- RFID reader module (e.g. MFRC522 or similar)
- Status indicators (LEDs / buzzer, etc.)
- Power supply for the ESP32 + reader
- 3D-printed enclosure (see `./enclosure/`)

> **Note:** Add your exact RFID module and wiring details here later.

---

## Firmware (ESP32)

The firmware is located in:

```text
./RFID/
```

Main responsibilities:

- Initialize RFID reader and read card UIDs
- Connect to Wi-Fi
- Send HTTP requests with card data to the backend
- Parse backend response (ALLOW / DENY)
- Drive output pins (LED/relay/buzzer) accordingly

> Later you can add a short section with build/flash instructions (PlatformIO, Arduino IDE, etc.).

---

## Backend (Node.js/Express + MongoDB)

The backend code is located in:

```text
./Server fo RFID/
```

Responsibilities:

- Expose HTTP endpoints for the ESP32 to call
- Validate RFID card IDs against MongoDB
- Log access attempts (timestamp, card ID, result)
- Return JSON responses to the ESP32

Basic run instructions (adjust to your actual project):

```bash
cd "Server fo RFID"
npm install
npm start
```

You will need a running MongoDB instance and a `.env` file with your connection string.

---

## Enclosure (3D Design)

The 3D model files for the device casing will be placed in:

```text
./enclosure/
```

Here you can include:

- `.stl` / `.step` files
- Photos of the printed and assembled enclosure (`./docs/images/enclosure-*.jpg`)

Example gallery:

```md
### Enclosure Photos

<p align="center">
  <img src="./docs/images/enclosure-front.jpg" width="320">
  <img src="./docs/images/enclosure-side.jpg" width="320">
</p>
```

---

## Project Status

This project is **functionally complete** but still being documented.

Planned improvements:

- Better documentation for wiring, APIs and setup
- Security improvements (e.g. authentication, HTTPS)
- Additional configuration options for access logic

---

## What I Learned

- Designing a small end-to-end system that connects **embedded hardware** to a **web backend**.
- Working with ESP32, RFID readers and HTTP-based communication.
- Building a Node.js/Express backend and integrating it with MongoDB.
- Handling real-time authorization logic and logging access events.
- Thinking about physical product design through a custom **3D-printed enclosure**.

---

## License

Released under the **MIT License**.  
See the `LICENSE` file for full details.

