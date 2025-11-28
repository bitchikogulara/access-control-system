# Access Control System

An ESP32-based RFID access control system featuring real-time validation through a Node.js/Express backend with MongoDB storage.  
Includes a configuration portal, 3D-printed enclosure, device display feedback, and complete wireless provisioning.

<p align="center">
  <img src="./docs/images/device-front.jpg" width="450" alt="Access Control System Device">
</p>

---

## Overview

This project implements a complete access control system using:

- ESP32 microcontroller  
- MFRC522 RFID reader  
- Node.js/Express backend  
- MongoDB database  
- Custom 3D-printed enclosure  
- Wi-Fi configuration portal with onboard AP mode  

The device reads RFID cards, sends their UID to the backend server, and reacts with **ALLOW** or **DENY** feedback based on server response.

---

## Features

- ESP32 firmware written for stable, real-time operation  
- MFRC522 RFID card reading  
- HTTP communication with backend  
- Node.js/Express REST API for validation and logging  
- MongoDB database for storing authorized cards and access logs  
- Device display + buzzer for user feedback  
- Full Wi-Fi provisioning system with captive-style config page  
- Lightweight 3D-printed enclosure designed for daily use  

---

## System Architecture

```text
[ RFID Card ] --> [ ESP32 + MFRC522 ] --> HTTP Request --> [ Node.js/Express Backend ] --> [ MongoDB ]
                                          <-- HTTP Response (ALLOW / DENY) <--
```

- Device reads RFID UID  
- Sends UID to backend  
- Backend checks database  
- Backend sends ALLOW / DENY  
- Device displays result + optional buzzer  

---

## Hardware

Components used:

- ESP32 Dev Board  
- MFRC522 RFID reader  
- Small I2C display  
- Buzzer  
- Custom 3D-printed enclosure  
- Reset button accessible through a pinhole in the enclosure  

The enclosure and all additional hardware photos will be placed inside:

```
./docs/images/
./enclosure/
```

---

## Firmware (ESP32)

Located in:

```
./RFID/
```

Responsible for:

- Initializing MFRC522 reader  
- Reading card UIDs  
- Connecting to Wi-Fi  
- Communicating with the backend via HTTP  
- Handling configuration mode  
- Non-volatile saving of Wi-Fi and server settings  
- Output control for display + buzzer  

---

## Backend (Node.js/Express + MongoDB)

Located in:

```
./Server for RFID/
```

Backend capabilities:

- Validates RFID card IDs  
- Provides `/validate` endpoint for device checking  
- Stores authorized cards  
- Logs all access attempts  
- Provides routes for viewing logs or card lists  
- JSON responses for device consumption  

### Running the backend

```bash
cd "Server for RFID"
npm install
npm start
```

**Requirements:**

- Running MongoDB instance  
- `.env` file containing:  
  ```
  MONGODB_URI=your_connection_string_here
  SERVER_PORT=your_port
  ```

---

## Enclosure (3D Design)

3D files stored in:

```
./enclosure/
```

Includes:

- `.stl` files  
- `.step` files  
- Build photos  
- Assembly instructions (optional)  

Example gallery (replace paths with your real images):

```md
### Enclosure Photos

<p align="center">
  <img src="./docs/images/enclosure-front.jpg" width="320">
  <img src="./docs/images/enclosure-side.jpg" width="320">
</p>
```

---

# Usage Instructions

## 1. Power On

When powered, the ESP32:

1. Loads saved Wi-Fi + server settings  
2. Attempts to connect  
3. Enters **normal operation** if successful  
4. Enters **Configuration Mode** if no valid settings exist  

---

# Normal Operation

1. Present an RFID card  
2. Device reads UID  
3. Sends request to backend  
4. Receives **ALLOW / DENY**  
5. Shows result on display + optional buzzer  
6. Backend logs the attempt  

> You may include a video demo here:  
> `./docs/videos/normal_attempt.mp4`

---

# Configuration Mode

Configuration mode enables network setup and server configuration.

You can enter config mode in **two ways**:

---

## A) Reset Button (5-Second Hold)

Your enclosure has a pinhole for reset access.

Procedure:

1. Insert a SIM-tool-style pin  
2. Hold the reset button for **5 seconds**  
3. Device reboots into configuration mode  
4. ESP32 creates a Wi-Fi AP:

```
SSID: DoorAccess
Password: 12341234
```

5. Connect to the AP  
6. Open browser → visit:

```
http://192.168.4.1
```

You will see the configuration interface where you can:

- Set Wi-Fi SSID  
- Set Wi-Fi password  
- Set backend server endpoint  

Settings are saved in non-volatile memory and used on next boot.

---

## B) Serial Command (USB)

You may also enter config mode via serial:

1. Connect via USB  
2. Open Serial Monitor @ 115200 baud  
3. Send command:

```
cfg
```

4. Device restarts into config mode  
5. Connect to `DoorAccess` and visit `192.168.4.1`

---

# Saving & Applying Settings

- All data is stored in **non-volatile memory**  
- Device auto-restarts after saving  
- On next boot, ESP32 connects and resumes normal mode  
- If connection fails repeatedly → fallback to config mode  

---

# Reset to Factory Defaults

Hold the reset button for **10+ seconds** to clear all saved configuration and restart in config mode.

(If your firmware uses a different duration, update this line.)

---

# What I Learned

- Designing a full end-to-end embedded + backend system  
- Working with ESP32, RFID modules, and HTTP communication  
- Building REST APIs using Node.js/Express  
- Managing data storage and access logs in MongoDB  
- Designing and iterating on a functional 3D-printed enclosure  
- Implementing non-volatile configuration systems (AP mode + captive portal)  

---

# License

Released under the **MIT License**.  
See the `LICENSE` file for full details.
