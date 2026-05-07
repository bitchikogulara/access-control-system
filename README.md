# Access Control System

An ESP32-based RFID access control system featuring real-time validation through a Node.js/Express backend with MongoDB storage.  
Includes a configuration portal, 3D-printed enclosure, device display feedback, and complete wireless provisioning.

<img height="500" alt="WhatsApp Image 2025-12-26 at 13 22 30" src="https://github.com/user-attachments/assets/1f51acbe-24b8-4132-b9b6-19feb98147ea" />
<img height="500" alt="WhatsApp Image 2025-12-26 at 13 21 58" src="https://github.com/user-attachments/assets/f3cd4ae2-81f4-4a41-a0bf-1d2a09d03ca3" />

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
- Wi-Fi provisioning via internal access point  
- Web-based configuration UI (both in AP mode and on normal LAN)  
- Custom 3D-printed enclosure designed for daily use  

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
- Small I2C display (SSD1306 128x64)  
- Buzzer  
- Reset/config button (accessible through enclosure pinhole)  
- Custom 3D-printed enclosure  

Enclosure photos and 3D files live in:

```text
./docs/images/
./enclosure/
```

---

## Firmware (ESP32)

Located in:

```text
./RFID/
```

Responsible for:

- Initializing MFRC522 and reading card UIDs  
- Connecting to Wi-Fi (STA mode)  
- Communicating with the backend via HTTP (JSON)  
- Handling configuration mode via:
  - Serial command (`cfg`)
  - Long press on reset button  
- Saving Wi-Fi + server settings in NVS (`Preferences`)  
- Providing user feedback on OLED + buzzer  
- Driving access control output pins  

---

## Backend (Node.js/Express + MongoDB)

Located in:

```text
./Server for RFID/
```

Backend capabilities:

- Validates RFID card UIDs  
- Provides API endpoint (e.g. `/api/cards/validate`) for device checks  
- Stores authorized cards and access logs in MongoDB  
- Returns JSON responses with `allow` flag and optional `cardholderName`  

### Running the backend

```bash
cd "Server for RFID"
npm install
npm start
```

**Requirements:**

- A running MongoDB instance  
- A `.env` file containing something like:  
  ```env
  MONGODB_URI=your_connection_string_here
  SERVER_PORT=3000
  ```

---

## Enclosure (3D Design)

coming soon!

# Usage Instructions

## 1. Power On

When powered, the ESP32:

1. Loads saved Wi-Fi + server settings from NVS  
2. Attempts to connect to the configured Wi-Fi network  
3. Enters **normal operation** once connected  

If settings are invalid, the device will still stay in normal loop, but you can always enter configuration mode manually (see below).

---

## 2. Normal Operation

1. Present an RFID card near the MFRC522 reader  
2. The device reads the UID and normalizes it  
3. Sends an HTTP POST request to `SERVER_URL` with UID, MAC, IP, RSSI, etc.  
4. Backend returns JSON with an `allow` flag and optional `card.cardholderName`  
5. Device behavior:
   - On **ALLOW**:
     - Plays “access granted” buzzer pattern  
     - Activates grant outputs (`SIGNAL_PIN_HIGH_1`, `SIGNAL_PIN_HIGH_2`, `SIGNAL_PIN_LOW`)  
     - Displays `Welcome` + cardholder name on OLED  
   - On **DENY**:
     - Plays “access denied” buzzer pattern  
     - Displays `Access Denied`  

Backend logs all attempts.

You can later attach a relay or lock to the signal pins.

---

# Configuration Mode

Configuration mode lets you set Wi-Fi SSID, password and backend URL.  
There are **two ways** to enter it: button or serial.

---

## A) Using the Reset Button (5-second Hold)

Your enclosure has a pinhole aligned with the reset/config button.

1. Insert a SIM-tool-style pin or thin rod  
2. Press and **hold** the button for about **5 seconds**  
3. The device:
   - Switches to AP mode  
   - Starts a config access point:
     ```text
     SSID: DoorConfig
     Password: 12341234
     ```
   - Starts the web UI on `192.168.4.1`  
   - Shows “configuration mode active” on the OLED  
   - Plays a triple config tone on the buzzer  

4. Connect your phone or laptop to the Wi-Fi network `DoorConfig`  
5. Open a browser and go to:

```text
http://192.168.4.1/
```

6. Log in with password: `12341234`  
7. Use the form to set:
   - Wi-Fi SSID  
   - Wi-Fi password  
   - Server URL (`SERVER_URL`)  

8. Click **Save & Reboot**  
   - Settings are written to NVS  
   - Device restarts and attempts to connect with the new settings  

---

## B) Using Serial Command (`cfg`)

You can also enter configuration mode via USB serial:

1. Connect the ESP32 to your computer  
2. Open a serial monitor (115200 baud)  
3. Type:

```text
cfg
```

and press Enter.

4. Device enters “serial configuration mode” and prompts you in the terminal for:
   - New Wi-Fi SSID  
   - New Wi-Fi password  
   - New server URL  

Press Enter on any field to keep the current value.

5. Confirm with `y` to save changes, or any other key to discard.  
6. On save:
   - Settings are written to NVS  
   - Device plays a “success” tone  
   - Device restarts using the new configuration  

---

## Web Admin UI on LAN

When the device is **already connected to your Wi-Fi network**, you can also access the configuration UI via its LAN IP:

1. Find the device IP address in your router or via serial logs  
2. Open:

```text
http://<device-ip>/
```

3. Log in with password:

```text
12341234
```

4. You’ll see the same configuration form as in AP mode (`/cfg` route).  
5. Saving configuration will also reboot the device.

---

## Notes on Behavior

- Wi-Fi reconnection is handled by `ensureWiFi()` with retries and error display.  
- If Wi-Fi or server are down, the device shows error messages and plays an error tone, but **you** decide when to enter config mode (via button or `cfg`).  
- Duplicate card reads within a short cooldown (`CARD_COOLDOWN_MS`) are ignored to avoid spamming the backend.

---

## What I Learned

- Designing a full end-to-end embedded + backend access control system  
- Interfacing ESP32 with MFRC522 and SSD1306 OLED  
- Implementing Wi-Fi connectivity, HTTP requests, and JSON handling on microcontrollers  
- Building REST APIs with Node.js/Express and integrating them with MongoDB  
- Using non-volatile storage (`Preferences`) for device configuration  
- Building a simple but effective web admin panel on an ESP32 using `WebServer`  
- Designing and printing a custom enclosure for an electronic device  

---

## License

Released under the **MIT License**.  
See the `LICENSE` file for full details.
