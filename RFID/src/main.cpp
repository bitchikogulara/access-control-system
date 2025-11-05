#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include <WebServer.h>  // simple admin web UI

Preferences prefs;   // NVS storage handle

// Default Wi-Fi and Server config values are initialised
// in initConfig() function (if not stored in NVC)
  // Server config
String SERVER_URL;   // backend endpoint for UID validation
  // Wi-Fi config
String WIFI_SSID;    // network SSID (from NVS)
String WIFI_PASSWORD;// network password (from NVS)

// tweak behavior
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 12000; // 12s initial attempt

// pin declarations
  // RC522 pins
#define SS_PIN 5        // MFRC522 SS
#define RST_PIN 27      // MFRC522 RST
  // Relay pins (replaced by signal pins)
#define SIGNAL_PIN_HIGH_1 12   // goes HIGH on access granted
#define SIGNAL_PIN_HIGH_2 2    // goes HIGH on access granted (duplicate)
#define SIGNAL_PIN_LOW    13   // goes LOW on access granted

// --- Buzzer pin ---
#define BZR_PIN 26       // piezo buzzer output

// --- Reset Button pin (using internal pull-up) ---
#define RESET_BTN_PIN 4    // connect button to GND and this pin

// RC522 config
MFRC522 mfrc522(SS_PIN, RST_PIN); // RFID reader instance

// Display config
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0); // 128x64 OLED

// Duplicate-read guard
static char  lastUid[16] = {0};   // up to 10 hex chars + NUL (MFRC522 4-byte UID = 8 hex; 7-byte UID = 14 hex)
static uint32_t lastSeenMs = 0;   // last successful UID timestamp
static const uint32_t CARD_COOLDOWN_MS = 1500; // ignore same card within 1.5s

// temporary buffer for reading comands from Serial 
static String _serialBuf;   // accumulates incoming serial until newline

// ---- Simple Admin Web UI (globals) ----
WebServer server(80);                           // HTTP server on port 80
static const char* ADMIN_PASS = "12341234";                 // admin password
static const uint32_t SESSION_MAX_AGE_SEC = 1800;           // cookie lifetime
volatile bool g_adminLock = false;                          // when true, pause normal processing

// ---- Config AP (for no-WiFi setup) ----
#define AP_SSID "DoorConfig"
#define AP_PASS "12341234"
static bool g_configAP = false;                             // true when AP config mode is active
volatile bool g_cfgSerialActive = false;                    // true while serial config menu is active

// Function Declarations
  // Wi-fi
void connectWiFi();                     // initial connection attempt
bool ensureWiFi(uint32_t retry_ms = 3000); // quick reconnect helper
  // Server Communication
bool sendUidToServer(const char* uid);  // POST UID to backend
  // RC522
bool readCard(char* outUid, size_t outLen); // read & normalize UID
  // Access behaviors
void accessGranted(const char* name);   // grant routine
void accessDenied();                    // deny routine
  // Screen
void displayLanding();                  // idle screen
void displayWait();                     // after scan, waiting
void displayDenied();                   // access denied
void displayServerError();              // backend error
void displayGranted(const char* name);  // access granted
void displayConnectingWifi();           // Wi-Fi connecting
void displayInvalidCard();              // invalid UID
void displayWifiError();                // Wi-Fi error
void displayConfigMode();               // config/admin mode banner
  // config menu control
bool checkForCfgCommand();              // parse "cfg" from Serial
void initConfigs(); // load values from NVS
bool cfgMode(); // open config menu, returns true if config changed.
void updateConfig(String tmpSSID,String tmpPASS,String tmpURL); // persist cfg
void endConfigMode();                   // exit admin/config modes
  // helpers
static String readLineWithTimeout(uint32_t msTimeout = 60000); // serial line read

// temp functions for Developent
  // initialize leds
void initPinmodes();                    // set signal pins

// -------- BUZZER: Declarations --------
void buzzerInit();                      // setup PWM channel
void buzz(uint16_t freq, uint16_t ms);  // play tone helper
void buzzScan();                        // short scan tone
void buzzGranted();                     // grant jingle
void buzzDenied();                      // deny jingle
void buzzError();                       // error jingle
void buzzClick();                       // UI click
void buzzConfig();                      // config triple tone
// ---------------------------------------------

// -------- Reset Button: Declarations --------
void initResetButton();                 // configure input
bool resetBtnPressed();                 // read state
// ---------------------------------------------

// -------- Admin Web UI: Declarations --------
bool isAuthed();                        // cookie check
void sendRedirect(const char* path);    // 302 helper
void handleRoot();                      // GET /
void handleLogin();                     // POST /login
void handleCfg();                       // GET /cfg
void handleSave();                      // POST /save
void startWebServer();                  // route bindings + start
// ---------------------------------------------

// -------- Config AP: Declarations --------
void startConfigAP();                   // start SoftAP for setup
// ---------------------------------------------

// -------- Button long/short press handler --------
void handleConfigButton();              // short=reset, long=AP or reboot
// ---------------------------------------------

void setup() {
  prefs.begin("cfg", false);           // open NVS namespace
  initConfigs();                        // load Wi-Fi/URL from NVS
  initPinmodes();                       // prepare signal outputs

  // Buzzer: initialize
  buzzerInit();                         // PWM + attach

  // Initialize reset button
  initResetButton();                    // enable reset/config button

  Wire.begin(21, 22);   // SDA = 21, SCL = 22  // I2C pins for OLED
  u8g2.begin();                          // init display

  Serial.begin(9600);                    // debug console
  SPI.begin();                           // SPI bus for RC522
  mfrc522.PCD_Init();                    // init RFID reader
  connectWiFi();                         // try initial Wi-Fi

  // start admin http server
  startWebServer();                      // enable web UI

  Serial.println("System Initialized.\nYou Can Scan Your Card Now...");
  if(!g_cfgSerialActive){
    displayLanding();                    // show idle screen
  }
}

void loop() {
  // Admin lock guard: when locked, only serve web requests
  if (g_adminLock) { 
    server.handleClient();               // service HTTP
    handleConfigButton();                // allow long-press reboot
    displayConfigMode();                 // keep banner
    delay(1); 
    return; 
  }

  // Manual reset button with long/short press logic
  handleConfigButton();                  // poll button

  displayLanding();                      // draw idle prompt

  if (checkForCfgCommand()) {
    // small click entering CFG
    buzzClick();                         // feedback

    Serial.println("opening CFG menu.");
    
    // restart the device if configuration changed successfully
    if(cfgMode()){
      Serial.println("Restarting The Device.");
      ESP.restart();                     // apply new cfg
    }
  }

  char uid[16] = {0}; // enough for common 4/7-byte UIDs in hex (+NUL). If you expect 10-byte UIDs, enlarge to 21.

  if (readCard(uid, sizeof(uid))) {
    uint32_t now = millis();             // timestamp scan
    displayWait();                       // show waiting

    // confirm scan
    buzzScan();                          // short beep

    // Ignore same UID within cooldown
    if (strcmp(uid, lastUid) == 0 && (now - lastSeenMs) < CARD_COOLDOWN_MS) {
      Serial.println("Duplicate read ignored.");
      return;
    }
    strncpy(lastUid, uid, sizeof(lastUid)); // remember last UID
    lastUid[sizeof(lastUid)-1] = '\0';      // force NUL
    lastSeenMs = now;                        // update time

    // Send to server
    if (!sendUidToServer(uid)) {
      Serial.println("Send failed. Will try again on next scan.");
    }
  }

  // serve http clients during normal operation
  server.handleClient();                  // process web requests

  // Small idle delay to reduce CPU usage
  delay(5);                               // cooperative yield
}



bool readCard(char* outUid, size_t outLen) {
  if (!mfrc522.PICC_IsNewCardPresent()) return false; // no new card
  if (!mfrc522.PICC_ReadCardSerial())   return false; // read failed

  // Build uppercase hex UID into a temporary buffer first
  // maxHexLen = 2 * uid.size
  size_t maxHexLen = (size_t)mfrc522.uid.size * 2; // computed but not directly used
  // Ensure we have room for at least 1 and NUL
  if (outLen < 2) {
    outUid[0] = '\0';
    return false;
  }

  // Use a local buffer large enough for typical UIDs (MFRC522 max uid.size is usually <= 10)
  // but we allocate conservatively up to 32 hex chars.
  char hexBuf[33];                        // temporary hex buffer
  size_t pos = 0;                         // write index

  for (byte i = 0; i < mfrc522.uid.size; ++i) {
    // make sure we don't overflow hexBuf (2 chars per byte + NUL)
    if (pos + 2 >= sizeof(hexBuf)) break; // guard
    // write two uppercase hex chars (no null yet)
    uint8_t b = mfrc522.uid.uidByte[i];
    const char hexDigits[] = "0123456789ABCDEF";
    hexBuf[pos++] = hexDigits[(b >> 4) & 0x0F]; // high nibble
    hexBuf[pos++] = hexDigits[b & 0x0F];        // low nibble
  }
  hexBuf[pos] = '\0';

  // Decide normalized result: server expects 8 hex chars (4 bytes)
  const size_t targetLen = 8;            // normalized length
  char norm[9]; // 8 chars + NUL
  if (pos >= targetLen) {
    // copy FIRST 8 chars
    memcpy(norm, &hexBuf[0], targetLen);
    norm[targetLen] = '\0';
  } else {
    // left-pad with '0' to 8 chars
    size_t pad = targetLen - pos;
    for (size_t i = 0; i < pad; ++i) norm[i] = '0';
    memcpy(&norm[pad], hexBuf, pos);
    norm[targetLen] = '\0';
  }

  // Ensure we can write result into outUid buffer
  if (outLen < (targetLen + 1)) {
    // not enough room — write truncated (but keep NUL)
    size_t copyLen = outLen - 1;
    memcpy(outUid, norm, copyLen);
    outUid[copyLen] = '\0';
  } else {
    memcpy(outUid, norm, targetLen + 1);
  }

  // Stop the card
  mfrc522.PICC_HaltA();                  // halt PICC
  mfrc522.PCD_StopCrypto1();             // stop encryption

  // Print
  Serial.print("UID: ");
  Serial.println(outUid);
  return true;                            // UID ready
}

void connectWiFi() {
  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
  displayConnectingWifi();                // show progress

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);        // don't write creds to flash repeatedly
  WiFi.setAutoReconnect(true);   // auto-reconnect when possible
  WiFi.setSleep(false);          // better latency & stability for HTTP/MQTT

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    Serial.print("IP: ");  Serial.println(WiFi.localIP());
    Serial.print("MAC: "); Serial.println(WiFi.macAddress());
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
    // sound success
    buzzGranted();
  } else {
    Serial.println("Initial Wi-Fi connect timed out. Will retry in loop when needed.");
    displayWifiError();
    // sound error
    buzzError();
    delay(2000);
  }
}

// Returns true if connected, else tries a quick reconnect.
bool ensureWiFi(uint32_t retry_ms) {
  if (WiFi.status() == WL_CONNECTED) return true; // already up

  Serial.println("Wi-Fi not connected. Reconnecting...");
  displayConnectingWifi();
  WiFi.reconnect();

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < retry_ms) {
    delay(200);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Reconnected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("Reconnect failed.");
  displayWifiError();
  delay(2000);
  return false;                              // still down
}

// helper to POST the UID
// ---- Tweak sendUidToServer() to handle non-2xx properly ----
bool sendUidToServer(const char* uid) {
  if (!ensureWiFi(4000)) return false;      // make sure Wi-Fi is up

  HTTPClient http;
  http.setTimeout(5000);                    // network timeout
  http.begin(SERVER_URL);                   // target endpoint
  http.addHeader("Content-Type", "application/json");

  // Build JSON safely (still manual but valid)
  String payload;
  payload.reserve(128);                     // avoid reallocs
  payload += "{\"card_uid\":\""; payload += uid; payload += "\",";
  payload += "\"mac\":\"";        payload += WiFi.macAddress(); payload += "\",";
  payload += "\"ip\":\"";         payload += WiFi.localIP().toString(); payload += "\",";
  payload += "\"rssi\":";         payload += String(WiFi.RSSI()); payload += "}";

  int code = http.POST(payload);            // send request
  if (code <= 0) {
    Serial.printf("HTTP error: %s\n", http.errorToString(code).c_str());
    http.end();
    displayServerError();
    // sound error
    buzzError();
    return false;                           // transport error
  }

  String body = http.getString();           // response body
  Serial.printf("HTTP %d | Body: %s\n", code, body.c_str());

  if (code < 200 || code >= 300) {
    Serial.println("Non-2xx from server. Treating as transient error.");
    http.end();
    displayServerError();
    // sound error
    buzzError();
    return false;                           // backend error
  }

  StaticJsonDocument<512> doc;              // parse JSON
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("JSON parse failed: "); Serial.println(err.c_str());
    http.end();
    displayServerError();
    // sound error
    buzzError();
    return false;                           // bad payload
  }

  bool allowed = doc["allow"] | false;     // decision flag
  const char* cardholderName = doc["card"]["cardholderName"] | ""; // optional name

  if (allowed) {
    Serial.println("ACCESS GRANTED");
    accessGranted(cardholderName);          // grant path
  } else {
    Serial.println("ACCESS DENIED");
    accessDenied();                         // deny path
  }

  http.end();
  return true;                              // request handled
}

// behavior when acces is granted
void accessGranted(const char* name){
  // sound granted
  buzzGranted();                            // audio feedback

  // Activate grant signals
  digitalWrite(SIGNAL_PIN_HIGH_1, HIGH);    // enable output 1
  digitalWrite(SIGNAL_PIN_HIGH_2, HIGH);    // enable output 2
  digitalWrite(SIGNAL_PIN_LOW,  LOW);       // active-low line

  displayGranted(name);                     // show welcome
  delay(10000);                             // keep door open window

  // Return to idle states
  digitalWrite(SIGNAL_PIN_HIGH_1, LOW);
  digitalWrite(SIGNAL_PIN_HIGH_2, LOW);
  digitalWrite(SIGNAL_PIN_LOW,  HIGH);

  displayLanding();                         // back to idle
  return;
}

// behavior when acces is denied
void accessDenied(){
  // sound denied
  buzzDenied();                             // audio feedback

  displayDenied();                          // show denied
  delay(3000);                              // brief pause
  displayLanding();                         // back to idle
  return;
}

// functions to Display messages on screen
// -----START-----
void displayLanding(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);
  int x = (128 - u8g2.getUTF8Width("Scan your card")) / 2; // center text
  u8g2.drawStr(x, 38, "Scan your card");
  u8g2.sendBuffer();
}

void displayWait(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  const char* text = "Card Scanned";
  int x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 21);
  u8g2.print(text);

  text = "Please Wait...";
  x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 42);
  u8g2.print(text);
  u8g2.sendBuffer();
}

void displayDenied(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);
  int x = (128 - u8g2.getUTF8Width("Access Denied.")) / 2;
  u8g2.drawStr(x, 38, "Access Denied.");
  u8g2.sendBuffer();
}

void displayServerError(){
  u8g2.setFont(u8g2_font_7x14B_tf);

  for(int timeLeft = 5; timeLeft >= 0; timeLeft--){
    u8g2.clearBuffer();
    const char* text = "Server Error";
    int x = (128 - u8g2.getUTF8Width(text)) / 2;
    u8g2.setCursor(x, 21);
    u8g2.print(text);

    text = "Try Again In ";
    x = (128 - u8g2.getUTF8Width(text)) / 2;
    u8g2.setCursor(x, 42);
    u8g2.print(text);
    u8g2.print(timeLeft);
    u8g2.sendBuffer();
    delay(1000);
  }
}

void displayGranted(const char* name){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  int x = (128 - u8g2.getUTF8Width("Welcome")) / 2;
  u8g2.setCursor(x, 21);
  u8g2.print("Welcome");

  x = (128 - u8g2.getUTF8Width(name)) / 2;
  u8g2.setCursor(x, 42);
  u8g2.print(name);
  u8g2.sendBuffer();
}

void displayConnectingWifi(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  const char* text = "Connecting";
  int x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 21);
  u8g2.print(text);

  text = "to WiFi...";
  x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 42);
  u8g2.print(text);

  u8g2.sendBuffer();
}

void displayInvalidCard(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  const char* text = "Invalid Card.";
  int x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 32);
  u8g2.print(text);
  u8g2.sendBuffer();
}

void displayWifiError(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  const char* text = "WiFi Error";
  int x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 21);
  u8g2.print(text);

  text = "Check Network";
  x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 42);
  u8g2.print(text);

  u8g2.sendBuffer();
}

void displayConfigMode(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);

  const char* text = "configuration mode";
  int x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 21);
  u8g2.print(text);

  text = "active";
  x = (128 - u8g2.getUTF8Width(text)) / 2;
  u8g2.setCursor(x, 42);
  u8g2.print(text);

  u8g2.sendBuffer();
}

// -----END-----

bool checkForCfgCommand() {
  // Read all available chars — but do NOT block
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;   // ignore CR

    if (c == '\n') {
      // A full line is ready
      String line = _serialBuf;
      _serialBuf = "";         // reset buffer

      line.trim();
      line.toLowerCase();

      if (line == "cfg") {
        return true;           // we found the command
      }
      return false;            // some other text
    }

    // Build up buffer
    if (_serialBuf.length() < 64) {
      _serialBuf += c;         // accumulate up to 64 chars
    }
  }
  return false;                // no full line yet
}

void initPinmodes(){
  pinMode(SIGNAL_PIN_HIGH_1, OUTPUT);
  pinMode(SIGNAL_PIN_HIGH_2, OUTPUT);
  pinMode(SIGNAL_PIN_LOW,  OUTPUT);

  // default idle states
  digitalWrite(SIGNAL_PIN_HIGH_1, LOW);
  digitalWrite(SIGNAL_PIN_HIGH_2, LOW);
  digitalWrite(SIGNAL_PIN_LOW,  HIGH);
}

void initConfigs(){
  SERVER_URL = prefs.getString("server_url", "http://X.X.X.X:3000/api/cards/validate"); // default URL
  WIFI_SSID     = prefs.getString("wifi_ssid", "DefSSID");                                     // default SSID
  WIFI_PASSWORD = prefs.getString("wifi_pass", "DefPASS");                                  // default password
}

bool cfgMode(){
  g_cfgSerialActive = true;               // block normal loop drawing
  Serial.println("Configuration Mode Active.");

  // tiny click on entering config 
  displayConfigMode();
  buzzConfig(); 

  String tmpSSID = WIFI_SSID;
  String tmpPASS = WIFI_PASSWORD;
  String tmpURL  = SERVER_URL;

  // Flush any stale input
  while (Serial.available()) (void)Serial.read();

  // request wifi_ssid
  Serial.printf("Provide new WiFi SSID, or press enter to keep current (%s):\n", WIFI_SSID.c_str());
  String input = readLineWithTimeout();
  if (input.length()) tmpSSID = input;

  // request wifi_pass
  Serial.printf("Provide new WiFi password, or press enter to keep current (%s):\n", WIFI_PASSWORD.c_str());
  input = readLineWithTimeout();
  if (input.length()) tmpPASS = input;

  // request server url
  Serial.printf("Provide new server url, or press enter to keep current (%s):\n", SERVER_URL.c_str());
  input = readLineWithTimeout();
  if (input.length()) tmpURL = input;

  // confirm
  Serial.println("Do you want to save modified information?");
  Serial.printf("WiFi SSID   : %s\n", tmpSSID.c_str());
  Serial.printf("WiFi Password: %s\n", tmpPASS.c_str());
  Serial.printf("Server URL  : %s\n", tmpURL.c_str());
  Serial.println("Press 'y' to save, 'n' to discard.");

  // wait for y/n with timeout
  char c = 0;
  uint32_t start = millis();
  while ((millis() - start) < 8000) {
    if (Serial.available()) {
      c = (char)Serial.read();
      if (c == '\r') continue;
      if (c == '\n') continue;
      break;
    }
    delay(5);
  }

  if (c == 'y' || c == 'Y') {
    updateConfig(tmpSSID,tmpPASS,tmpURL);  // save to NVS

    // sound success on save
    buzzGranted();
    g_cfgSerialActive = false;

    return true;  // trigger restart in caller
  }

  Serial.println("Configuration change discarded.");

  // sound deny on discard
  buzzDenied();
  g_cfgSerialActive = false;

  return false;
}

void endConfigMode(){
  if (g_configAP) {
    WiFi.softAPdisconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  }
  g_configAP = false;
  g_adminLock = false;
  displayLanding();
}

static String readLineWithTimeout(uint32_t msTimeout) {
  uint32_t start = millis();
  String line;
  while ((millis() - start) < msTimeout) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\r') continue;
      if (c == '\n') { line.trim(); return line; }
      if (line.length() < 128) line += c;
    }
    delay(5);
  }
  line.trim();
  return line; // may be empty on timeout
}


// ================= BUZZER IMPLEMENTATION =================
static const int BUZZER_CH   = 0;       // LEDC channel 0
static const int BUZZER_RES  = 10;      // 10-bit resolution
static const int BUZZER_DUTY = 512;     // ~50% duty (0..1023 for 10-bit)

void buzzerInit() {
  pinMode(BZR_PIN, OUTPUT); // safe default
  ledcSetup(BUZZER_CH, 1000 /*Hz*/, BUZZER_RES); // PWM setup
  ledcAttachPin(BZR_PIN, BUZZER_CH);             // bind pin to channel
  ledcWrite(BUZZER_CH, 0); // off
}

// Play a tone for `ms` milliseconds. freq=0 turns it off.
void buzz(uint16_t freq, uint16_t ms) {
  if (freq == 0) {
    ledcWriteTone(BUZZER_CH, 0);
    ledcWrite(BUZZER_CH, 0);
    return;
  }
  ledcWriteTone(BUZZER_CH, freq);      // set tone
  ledcWrite(BUZZER_CH, BUZZER_DUTY);   // enable duty
  delay(ms);                            // hold duration
  ledcWrite(BUZZER_CH, 0); // stop
}

// Tiny patterns
void buzzScan() {            // short “pip”
  buzz(2000, 80);
}
void buzzGranted() {         // two rising beeps
  buzz(1500, 80); delay(40);
  buzz(2200, 120);
}
void buzzDenied() {          // two low beeps
  buzz(600, 120); delay(40);
  buzz(450, 180);
}
void buzzError() {           // descending
  buzz(1200, 140); delay(40);
  buzz(800, 160);
}
void buzzClick() {           // tiny UI click
  buzz(1800, 40);
}
void buzzConfig() {         // short triple tone to indicate config mode
  buzz(1200, 80); delay(40);
  buzz(1700, 80); delay(40);
  buzz(1200, 100);
}

// ================================================================

// ================= RESET BUTTON IMPLEMENTATION =================
void initResetButton() {
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);  // idle HIGH, pressed = LOW
}

bool resetBtnPressed() {
  return digitalRead(RESET_BTN_PIN) == LOW; // true when pressed
}
// ======================================================================

// ================= ADMIN WEB UI IMPLEMENTATION =================
bool isAuthed() {
  if (!server.hasHeader("Cookie")) return false; // no cookie
  String cookie = server.header("Cookie");
  return cookie.indexOf("auth=1") >= 0;         // basic check
}

void sendRedirect(const char* path) {
  server.sendHeader("Location", path);
  server.send(302, "text/plain", "");
}

// GET /
void handleRoot() {
  if (isAuthed()) { sendRedirect("/cfg"); return; }
  server.send(200, "text/html", R"HTML(
<!doctype html>
<html>
  <head><meta charset="utf-8"><title>Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>body{font-family:sans-serif;margin:32px;max-width:520px}</style></head>
  <body>
    <h2>Device Login</h2>
    <form method="POST" action="/login">
      <label>Password:</label><br>
      <input type="password" name="pw" required autofocus>
      <button type="submit">Enter</button>
    </form>
  </body>
</html>
)HTML");
}

// POST /login
void handleLogin() {
  String pw = server.hasArg("pw") ? server.arg("pw") : ""; // read form
  if (pw == ADMIN_PASS) {
    String cookie = "auth=1; Max-Age=" + String(SESSION_MAX_AGE_SEC) + "; Path=/; SameSite=Lax; HttpOnly";
    server.sendHeader("Set-Cookie", cookie);
    g_adminLock = true;  // keep if you want to pause normal processing
    sendRedirect("/cfg");    server.sendHeader("Set-Cookie", cookie);
    g_adminLock = true;  // pause device
    sendRedirect("/cfg");
    return;
  }
  server.send(401, "text/html", R"HTML(
<!doctype html>
<html><head><meta charset="utf-8"><title>Login Failed</title></head>
<body>
  <p>Wrong password.</p>
  <a href="/">Try again</a>
</body></html>
)HTML");
}

// GET /cfg  (protected)
void handleCfg() {
  if (!isAuthed()) { sendRedirect("/"); return; }
  String page = R"HTML(
<!doctype html>
<html>
  <head><meta charset="utf-8"><title>Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>body{font-family:sans-serif;margin:32px;max-width:720px}label{display:block;margin-top:12px}</style></head>
  <body>
    <h2>Device Configuration</h2>
    <form method="POST" action="/save">
      <label>Wi-Fi SSID
        <input name="ssid" required value=")HTML";
  page += WIFI_SSID;
  page += R"HTML("></label>
      <label>Wi-Fi Password
        <input name="wpass" required value=")HTML";
  page += WIFI_PASSWORD;
  page += R"HTML("></label>
      <label>Server URL
        <input name="url" required value=")HTML";
  page += SERVER_URL;
  page += R"HTML("></label>
      <div style="margin-top:16px">
        <button type="submit">Save & Reboot</button>
        <button type="submit" formaction="/cancel" formmethod="POST">Discard</button>
      </div>
    </form>
    <p style="margin-top:24px"><a href="/logout">Log out</a></p>
  </body>
</html>
)HTML";
  server.send(200, "text/html", page);
}

// POST /save  (protected)
void handleSave() {
  if (!isAuthed()) { sendRedirect("/"); return; }

  String ssid  = server.hasArg("ssid")  ? server.arg("ssid")  : WIFI_SSID;   // new SSID
  String wpass = server.hasArg("wpass") ? server.arg("wpass") : WIFI_PASSWORD; // new pass
  String url   = server.hasArg("url")   ? server.arg("url")   : SERVER_URL;    // new URL

  updateConfig(ssid, wpass, url);       // persist to NVS
  Serial.println("Web save complete. Rebooting...");

  server.send(200, "text/html", R"HTML(
<!doctype html>
<html><head><meta charset="utf-8"><title>Saved</title>
<meta http-equiv="refresh" content="4;url=/"></head>
<body><p>Configuration saved. Rebooting device…</p></body></html>
)HTML");

  delay(400);
  ESP.restart();                         // reboot to apply
}

void startWebServer() {
  // collect Cookie header so isAuthed() works
  const char* headerKeys[] = { "Cookie" };
  size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
  server.collectHeaders(headerKeys, headerKeysCount);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/cfg", HTTP_GET, handleCfg);
  server.on("/save", HTTP_POST, handleSave);

  // discard changes and resume
  server.on("/cancel", HTTP_POST, [](){
    if (!isAuthed()) { sendRedirect("/"); return; }
    server.sendHeader("Set-Cookie", "auth=; Max-Age=0; Path=/; SameSite=Lax; HttpOnly");
    g_adminLock = false;  // resume
    endConfigMode();
    server.send(200, "text/html", R"HTML(
<!doctype html><html><body>
<p>Changes discarded. Device resumed.</p>
<meta http-equiv="refresh" content="1;url=/">
</body></html>
)HTML");
  });

  // logout endpoint
  server.on("/logout", HTTP_GET, [](){
    server.sendHeader("Set-Cookie", "auth=; Max-Age=0; Path=/; SameSite=Lax; HttpOnly");
    g_adminLock = false;  // resume
    endConfigMode();
    sendRedirect("/");
  });

  // not found
  server.onNotFound([](){
    if (!isAuthed() && server.uri() != "/" && server.uri() != "/login") { sendRedirect("/"); return; }
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Admin Web UI started on http://<device-ip>/");
}
// ======================================================================

// ================= CONFIG AP IMPLEMENTATION =================
void startConfigAP() {
  WiFi.disconnect(true);               // drop STA
  delay(100);
  WiFi.mode(WIFI_AP);                  // AP only during config
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP start: "); Serial.println(ok ? "OK" : "FAIL");
  Serial.print("AP SSID: "); Serial.println(AP_SSID);
  Serial.print("AP PASS: "); Serial.println(AP_PASS);
  Serial.print("AP IP  : "); Serial.println(ip); // 192.168.4.1 by default
  // admin page stays on the same server instance
  Serial.println("Open http://192.168.4.1/ to configure.");

  displayConfigMode();
  buzzConfig();  
}
// ======================================================================

// ================= BUTTON LONG/SHORT PRESS HANDLER =================
void handleConfigButton(){
  static bool wasDown = false;            // previous state
  static uint32_t downAt = 0;             // press timestamp
  static bool longActionDone = false;     // guard to avoid double-fire

  bool down = resetBtnPressed();          // current state

  // edge: pressed
  if (down && !wasDown) {
    downAt = millis();
    longActionDone = false;
    buzzClick(); // tiny UI click
  }

  // while held: check for long hold (5s)
  if (down && !longActionDone) {
    uint32_t held = millis() - downAt;

    // if already in config/admin-lock, long press reboots
    if (held >= 5000 && (g_configAP || g_adminLock)) {
      Serial.println("Long press during config/admin lock. Rebooting...");
      buzzClick();                       // tiny UI haptic
      delay(120);
      ESP.restart();
    }

    // original behavior: enter AP config if not already in AP mode
    if (held >= 5000 && !g_configAP && !g_adminLock) {      // 5 seconds => enter AP config
      Serial.println("Reset button held 5s. Starting Config AP.");
      startConfigAP();                       // start AP 192.168.4.1
      g_adminLock = true;                    // pause device (RFID etc.)
      g_configAP = true;                     // mark AP mode active
      longActionDone = true;                 // consume long action
    }
  }

  // edge: released
  if (!down && wasDown) {
    uint32_t held = millis() - downAt;

    // released before 5s and long action not taken => short press reboot
    if (!longActionDone && held >= 50 && !g_configAP) { // simple debounce, skip if AP already taken
      Serial.println("Short reset button press. Rebooting...");
      buzzClick();                       // tiny UI haptic
      delay(120);
      ESP.restart();
    }
  }

  wasDown = down;                          // update edge memory
}

// ======================================================================

void updateConfig(String tmpSSID,String tmpPASS,String tmpURL){
    WIFI_SSID     = tmpSSID;               // apply new SSID
    WIFI_PASSWORD = tmpPASS;               // apply new pass
    SERVER_URL    = tmpURL;                // apply new backend URL

    prefs.putString("wifi_ssid", WIFI_SSID);
    prefs.putString("wifi_pass", WIFI_PASSWORD);
    prefs.putString("server_url", SERVER_URL);
    Serial.println("New configuration saved.");

    return;
}
