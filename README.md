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
- The device reacts accordingly (LED/relay/buzzer/etc.).  

---

## Hardware

Main components:

- ESP32 development board  
- RFID reader module (MFRC522 or similar)  
- Status indicators (LEDs / buzzer / relay)  
- 3D-printed enclosure (see `/enclosure/`)  
- Power supply for ESP32 + reader  

> **Note:** Add exact wiring details when ready.

---

## Firmware (ESP32)

Located in:

```text
./RFID/
```

Responsibilities:

- Initialize RFID reader and read card UIDs  
- Connect to Wi-Fi  
- Send HTTP POST requests to backend  
- Process ALLOW/DENY responses  
- Trigger actuator output  

---

## Backend (Node.js/Express + MongoDB)

Located in:

```text
./Server fo RFID/
```

Responsibilities:

- Provide API endpoints for ESP32  
- Validate card IDs against MongoDB  
- Log all access attempts  
- Respond with ALLOW/DENY  

Run instructions:

```bash
cd "Server fo RFID"
npm install
npm start
```

Requires a working MongoDB instance + `.env` file.

---

## Enclosure (3D Design)

Files will be placed in:

```text
./enclosure/
```

Include:

- `.stl` / `.step` CAD files  
- Photos of printed enclosure  

Example gallery section:

```md
### Enclosure Photos

<p align="center">
  <img src="./docs/images/enclosure-front.jpg" width="320">
  <img src="./docs/images/enclosure-side.jpg" width="320">
</p>
```

---

## Project Status

The system is **functionally complete**.  
Documentation updates are ongoing.

Planned improvements:

- Add wiring schematics  
- Add full API reference  
- Improve security (HTTPS, API keys)  
- Add device configuration endpoints  

---

## What I Learned

- Designing full end-to-end hardware + backend systems  
- Working with ESP32, RFID modules, and network communication  
- Building REST APIs with Node.js/Express and MongoDB  
- Managing real-time authorization and event logging  
- Designing enclosures for 3D printing  

---

## License

Add your preferred license (MIT recommended)
