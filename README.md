# Access Control System

ESP32-based RFID access control system with a Node.js/Express backend and MongoDB storage.  
Supports card validation, logging, and real-time authorization over HTTP.

<p align="center">
  <img src="./docs/images/device-front.jpg" width="450" alt="Access Control System Device">
</p>

---

## Overview

This project is a complete access control system built around an ESP32 microcontroller and an MFRC522 RFID reader.  
The ESP32 reads RFID cards, sends authorization requests to a backend server over HTTP, and unlocks or denies access based on the server’s response.

The system includes:

- **Embedded firmware** running on an ESP32  
- **Backend server** built with Node.js/Express and MongoDB  
- **3D-printed enclosure** for the final physical product  

---

## Features

- ESP32-based controller with MFRC522 RFID reader  
- HTTP communication between device and backend  
- Node.js/Express backend for card validation and event logging  
- MongoDB database for storing authorized cards and logs  
- Real-time authorization response (ALLOW / DENY)  
- Custom-designed 3D-printed enclosure  

---

## System Architecture

```text
[ RFID Card ] --> [ ESP32 + RFID Reader ] --> HTTP Request --> [ Node.js/Express Backend ] --> [ MongoDB ]
                                          <-- HTTP Response (ALLOW / DENY) <--
```

---

## Hardware

Main components:

- ESP32 development board  
- MFRC522 RFID reader module  
- Small display + buzzer for status feedback  
- 3D-printed enclosure (see `./enclosure/`)  

---

## Firmware (ESP32)

Located in:

```text
./RFID/
```

Main responsibilities:

- Initialize RFID reader and capture card UIDs  
- Connect to Wi-Fi  
- Send UID to backend via HTTP  
- Parse backend response (ALLOW / DENY)  
- Activate output devices (display, buzzer, etc.)  

---

## Backend (Node.js/Express + MongoDB)

Located in:

```text
./Server for RFID/
```

Responsibilities:

- Provide REST API endpoints  
- Validate RFID UIDs against MongoDB  
- Log access attempts with timestamps  
- Respond to firmware with JSON (ALLOW / DENY)  

Run instructions:

```bash
cd "Server for RFID"
npm install
npm start
```

Requires:  
- A running MongoDB instance  
- A `.env` file containing your MongoDB connection string  

---

## Enclosure (3D Design)

3D model files are located in:

```text
./enclosure/
```

Includes:

- `.stl` and/or `.step` files  
- Photos of printed and assembled enclosure (`./docs/images/enclosure-*.jpg`)  

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

This project is **functionally complete** and the code is stable.  
Documentation updates and small refinements are planned.

---

## What I Learned

- Designing an end-to-end system connecting **embedded hardware** to a **web backend**  
- Working with ESP32, MFRC522, and HTTP-based communication  
- Building REST APIs with Node.js/Express and integrating with MongoDB  
- Implementing real-time decision logic for access control  
- Designing and printing a custom enclosure for physical integration  

---

## License

Released under the **MIT License**.  
See the `LICENSE` file for full details.
