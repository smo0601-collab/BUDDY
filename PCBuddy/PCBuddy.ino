/*
  =====================================================================
   PCBuddy v1.3 – Desk-Companion mit Web-UI
   Board : ESP32-C3 SuperMini (Arduino-ESP32 Core 3.x)
   Display: 0,96" OLED I2C SSD1306 128x64  ODER  1,3" OLED I2C SH1106 128x64
   Lib   : U8g2 (Bibliotheksverwalter: "U8g2" von oliver)

   Funktionen
   - animierte Augen mit 21 Emotionen, Blinzeln, Umherschauen, automatischer Durchlauf
   - Touch-Gesten: Tippen, Doppeltipp (Info), 3x (Ruhemodus), lang (Love), 5 s (Setup)
   - Neigungssensor: Schütteln macht PCBuddy wütend (weiter schütteln = Feuer),
     langsames Hin- und Herneigen gibt ein "Uh"-Gesicht (lange schaukeln = schwindelig)
   - Info-Folien: Wetter (Open-Meteo), Pac-Man-Uhr (NTP), WLAN, IP, Firmware – bleiben bis Doppeltipp
   - Lauftext (Geschwindigkeit einstellbar) & stehender Text mit Umlauten/€
   - Dimmen, Ruhemodus nach Zeitplan, Nachtuhr, Display aus
   - WLAN-Setup-Portal, mDNS (name.local), Web-Dashboard, HTTP-API, USB-Seriell
  =====================================================================
*/
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include "web_ui.h"

// =====================================================================
//  Typen & globale Zustände
// =====================================================================
struct Settings {
  String ssid, pass, host, tz, city;
  String updPw;                      // optionales Passwort für Updates per WLAN
  uint8_t oled = 0;          // 0 = 0,96" SSD1306, 1 = 1,3" SH1106
  bool    flip = false;
  uint8_t bright = 80;       // Helligkeit 1..100 %
  // Ruhemodus
  bool     quietOn = false;          // nach Zeitplan
  uint16_t quietFrom = 22 * 60;      // Minuten ab 0:00
  uint16_t quietTo = 7 * 60;
  uint8_t  quietBright = 5;          // %
  bool     quietMute = true, quietNoVibe = true;
  uint8_t  quietDisp = 1;            // 0 Gesicht gedimmt, 1 schlafendes Gesicht, 2 Pac-Man-Uhr, 3 Display aus
  float   lat = DEFAULT_LAT, lon = DEFAULT_LON;
  bool slWeather = true, slClock = true, slWifi = true, slIp = true, slFw = true;
  bool lockInfo = false;
  bool sound = true, vibe = true;
  bool pacSound = false;             // "Waka-Waka" beim Ziffern-Fressen
  bool shakeOn = true;               // Schütteln macht wütend
  uint8_t shakeSens = 3;             // Empfindlichkeit 1 (wenig) .. 5 (sehr)
  // Automatischer Emotions-Wechsel
  uint8_t  cycleMode = 0;            // 0 aus, 1 Durchlauf, 2 gelegentlich im Leerlauf
  uint8_t  cycleOrder = 0;           // 0 der Reihe nach, 1 zufällig
  uint8_t  cycleSec = 6;             // Sekunden pro Emotion bzw. Pause im Leerlauf
  uint32_t emoMask = 0xFFFFFFFF;     // welche Emotionen mitmachen (Bit = Emotion)
  uint8_t mqSpeed = 5;               // Lauftext 1..10
  uint8_t sleepMin = 0;              // 0 = nie
};

enum Mode : uint8_t { M_FACE, M_INFO, M_MARQUEE, M_SLEEP, M_PORTAL, M_MSG, M_TEXT, M_CLOCK, M_OFF, M_UPDATE };
const char* const MODE_NAMES[] = {"Gesicht", "Info", "Lauftext", "Schlaf", "Setup-Portal", "Meldung",
                                  "Text", "Uhr", "Display aus", "Update"};

enum Emotion : uint8_t {
  E_NEUTRAL, E_SERIOUS, E_SLEEPY, E_CRYING, E_SAD, E_CUTE, E_KISS, E_MINILOVE,
  E_ANIMELOVE, E_FIRE, E_ANGRY, E_LEFT, E_RIGHT, E_LOVE,
  E_HEART, E_SURPRISED, E_DIZZY, E_LAUGH, E_STAR, E_CONFUSED, E_UH, E_COUNT
};
const char* const EMO_NAMES[E_COUNT] = {
  "neutral", "serious", "sleepy", "crying", "sad", "cute", "kissou", "mini_love",
  "anime_love", "fire", "angry", "left", "right", "love",
  "heartbeat", "surprised", "dizzy", "laugh", "star", "confused", "uh"
};

enum Slide : uint8_t { S_WEATHER, S_CLOCK, S_WIFI, S_IP, S_FW, S_COUNT };

struct Particle { float x, y, vx, vy; int16_t life; uint8_t type; }; // 0 Herz, 1 z, 2 Funkeln
struct Note { uint16_t f, ms; };
struct PacWP { int16_t x, y; bool eat; };          // Wegpunkt der Pac-Man-Uhr
enum PacState : uint8_t { PAC_ROAM, PAC_EAT };

Settings    cfg;
Preferences prefs;
WebServer   server(80);
DNSServer   dns;
U8G2*       d = nullptr;

Mode     mode = M_FACE;
bool     portalActive = false;
uint32_t portalSince = 0;
bool     mdnsStarted = false;
uint32_t restartAt = 0;
uint32_t lastActivity = 0;

// ---- Gesicht ----
Emotion  emo = E_NEUTRAL;
uint32_t emoUntil = 0, emoStart = 0;
Mode     emoReturn = M_FACE;        // nach Ablauf der Emotion hierhin zurück (z. B. Info/Uhr)
uint32_t cycleLast = 0;
int8_t   cycleIdx = -1;
bool     cycleSet = false;          // aktuelle Emotion stammt vom Durchlauf (nicht manuell gewählt)

// ---- Neigungssensor (Schütteln) ----
bool     tiltState = false;
uint32_t tiltEdges[16];
uint8_t  tiltPos = 0;
uint32_t tiltLastEdge = 0, lastShake = 0;
uint8_t  shakeCombo = 0;
uint32_t rockEvents[8];           // langsame Kipp-Wechsel (für das "Uh"-Gesicht)
uint8_t  rockPos = 0, rockCombo = 0;
uint32_t lastRock = 0;
float    curW = 36, curH = 36, curY = 0, offX = 0, offY = 0, tgtX = 0, tgtY = 0;
uint32_t nextBlink = 0, blinkStart = 0, nextGlance = 0, nextSpawn = 0;
Particle parts[12];

// ---- Info-Folien ----
uint8_t  slides[S_COUNT];
uint8_t  slideCount = 0, slideIdx = 0;

// ---- Meldung ----
String   msgL1, msgL2, msgL3;
uint32_t msgUntil = 0;
Mode     msgReturn = M_FACE;   // wohin nach der Meldung (Info/Uhr bleiben erhalten)

// ---- Lauftext ----
String   mqText;
bool     mqLoop = false;
float    mqPos = 128;
int      mqW = 0;
uint32_t mqLast = 0;

// ---- Statischer Text ----
#define TXT_MAX_LINES 6
String   txtBuf[TXT_MAX_LINES];
uint8_t  txtCount = 0, txtFont = 0;
uint32_t txtUntil = 0;

// ---- Wetter (wird von eigenem Task geschrieben) ----
volatile bool  wxValid = false, wxForce = true;
volatile float wxTemp = 0, wxWind = 0;
volatile int   wxCode = -1, wxHum = -1;
volatile uint32_t wxLast = 0;

// ---- Touch ----
bool     tState = false, longFired = false, setupFired = false;
uint32_t tChange = 0, tPressAt = 0, tReleaseAt = 0;
uint8_t  tapCount = 0;

// ---- Ton & Vibration ----
Note     nq[10];
uint8_t  nqLen = 0, nqPos = 0;
uint32_t nqNext = 0, vibeOff = 0;

// ---- Helligkeit & Ruhemodus ----
int8_t   quietOverride = -1;   // -1 = Zeitplan, 0 = erzwungen aus, 1 = erzwungen an
bool     quietLastSched = false, quietNow = false, quietScreen = false;
int      panelLevel = -1;
bool     deepDim = false;

// ---- Pac-Man-Uhr ----
int8_t   pcShown[4] = {-1, -1, -1, -1};   // angezeigte Ziffern
uint8_t  pcMask[4][7];                    // sichtbare Pellets je Zeile
bool     pcChanged[4];
int8_t   pcNew[4];
uint32_t pcLastDraw = 0, pcLastStep = 0, pcLastWaka = 0;
bool     pcWakaHi = false;
bool     pcWait = false;          // gefressen, warte auf den Minutenwechsel
int8_t   pcNow[4];                // aktuelle Ziffern (für den Aufbau-Zeitpunkt)
uint32_t pcWaitSince = 0;

// ---- Seriell ----
String   serBuf;

// =====================================================================
//  Hilfsfunktionen
// =====================================================================
String jsonEsc(const String& s) {
  String o; o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
    else o += c;
  }
  return o;
}

String sanitizeHost(String h) {
  h.toLowerCase();
  if (h.endsWith(".local")) h.remove(h.length() - 6);
  String o;
  for (size_t i = 0; i < h.length() && o.length() < 24; i++) {
    char c = h[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') o += c;
  }
  while (o.startsWith("-")) o.remove(0, 1);
  while (o.endsWith("-")) o.remove(o.length() - 1);
  return o.length() ? o : String(DEFAULT_HOST);
}

// Vergangene Zeit seit t – robust, auch wenn t minimal in der "Zukunft" liegt
uint32_t since(uint32_t t) { int32_t dd = (int32_t)(millis() - t); return dd < 0 ? 0 : (uint32_t)dd; }

// Ersetzt Zeichen, die die Display-Schriften nicht haben, durch passende Alternativen
String normalizeText(const String& in) {
  String s = in;
  struct R { const char* from; const char* to; };
  static const R REP[] = {
    {"\xE2\x80\x9E", "\""}, {"\xE2\x80\x9C", "\""}, {"\xE2\x80\x9D", "\""}, {"\xC2\xAB", "\""}, {"\xC2\xBB", "\""},
    {"\xE2\x80\x9A", "'"}, {"\xE2\x80\x98", "'"}, {"\xE2\x80\x99", "'"}, {"\xC2\xB4", "'"}, {"`", "'"},
    {"\xE2\x80\x93", "-"}, {"\xE2\x80\x94", "-"}, {"\xE2\x88\x92", "-"}, {"\xE2\x80\xA6", "..."},
    {"\xE2\x80\xA2", "*"}, {"\xE2\x84\xA2", "TM"}, {"\xE2\x86\x92", "->"}, {"\xE2\x86\x90", "<-"},
    {"\xE2\x9D\xA4\xEF\xB8\x8F", "<3"}, {"\xE2\x9D\xA4", "<3"}, {"\xE2\x99\xA5", "<3"},
    {"\xF0\x9F\x99\x82", ":)"}, {"\xF0\x9F\x98\x8A", ":)"}, {"\xF0\x9F\x98\x80", ":D"}, {"\xF0\x9F\x98\x82", ":'D"},
    {"\xF0\x9F\x98\x89", ";)"}, {"\xF0\x9F\x98\xA2", ":'("}, {"\xF0\x9F\x99\x81", ":("}, {"\xF0\x9F\x91\x8D", "(y)"},
    {"\xEF\xB8\x8F", ""}, {"\xC2\xA0", " "}, {"\t", " "}, {"\r", ""}, {"\n", " "},
  };
  for (auto& r : REP) s.replace(r.from, r.to);
  String o; o.reserve(s.length());
  for (size_t i = 0; i < s.length();) {           // übrige Emojis (4-Byte-UTF-8) entfernen
    uint8_t c = s[i];
    if (c >= 0xF0) { i += 4; continue; }
    o += (char)c; i++;
  }
  return o;
}

bool timeValid() { return time(nullptr) > 1700000000; }
bool wifiOk()    { return WiFi.status() == WL_CONNECTED; }
void touchActivity() { lastActivity = millis(); }

// =====================================================================
//  Einstellungen (NVS / Preferences)
// =====================================================================
void loadSettings() {
  prefs.begin("pcbuddy", false);
  cfg.ssid        = prefs.getString("ssid", "");
  cfg.pass        = prefs.getString("pass", "");
  cfg.host        = sanitizeHost(prefs.getString("host", DEFAULT_HOST));
  cfg.tz          = prefs.getString("tz", DEFAULT_TZ);
  cfg.city        = prefs.getString("city", DEFAULT_CITY);
  cfg.updPw       = prefs.getString("updPw", "");
  cfg.oled        = prefs.getUChar("oled", 0);
  cfg.flip        = prefs.getBool("flip", false);
  cfg.bright      = constrain(prefs.getUChar("bright", 80), 1, 100);
  cfg.quietOn     = prefs.getBool("qOn", false);
  cfg.quietFrom   = prefs.getUShort("qFrom", 22 * 60) % 1440;
  cfg.quietTo     = prefs.getUShort("qTo", 7 * 60) % 1440;
  cfg.quietBright = constrain(prefs.getUChar("qBright", 5), 1, 100);
  cfg.quietMute   = prefs.getBool("qMute", true);
  cfg.quietNoVibe = prefs.getBool("qNoVibe", true);
  cfg.quietDisp   = min<uint8_t>(prefs.getUChar("qDisp", 1), 3);
  cfg.lat         = prefs.getFloat("lat", DEFAULT_LAT);
  cfg.lon         = prefs.getFloat("lon", DEFAULT_LON);
  cfg.slWeather   = prefs.getBool("slWx", true);
  cfg.slClock     = prefs.getBool("slClk", true);
  cfg.slWifi      = prefs.getBool("slWifi", true);
  cfg.slIp        = prefs.getBool("slIp", true);
  cfg.slFw        = prefs.getBool("slFw", true);
  cfg.lockInfo    = prefs.getBool("lockInfo", false);
  cfg.sound       = prefs.getBool("sound", true);
  cfg.vibe        = prefs.getBool("vibe", true);
  cfg.pacSound    = prefs.getBool("pacSound", false);
  cfg.shakeOn     = prefs.getBool("shakeOn", true);
  cfg.shakeSens   = constrain(prefs.getUChar("shakeSens", 3), 1, 5);
  cfg.cycleMode   = min<uint8_t>(prefs.getUChar("cycMode", 0), 2);
  cfg.cycleOrder  = prefs.getUChar("cycOrder", 0) ? 1 : 0;
  cfg.cycleSec    = constrain(prefs.getUChar("cycSec", 6), 2, 60);
  cfg.emoMask     = prefs.getUInt("emoMask", 0xFFFFFFFF);
  cfg.mqSpeed     = constrain(prefs.getUChar("mqSpeed", 5), 1, 10);
  cfg.sleepMin    = prefs.getUChar("sleepMin", 0);
  prefs.end();
}

void saveSettings() {
  prefs.begin("pcbuddy", false);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  prefs.putString("host", cfg.host);
  prefs.putString("tz", cfg.tz);
  prefs.putString("city", cfg.city);
  prefs.putString("updPw", cfg.updPw);
  prefs.putUChar("oled", cfg.oled);
  prefs.putBool("flip", cfg.flip);
  prefs.putUChar("bright", cfg.bright);
  prefs.putBool("qOn", cfg.quietOn);
  prefs.putUShort("qFrom", cfg.quietFrom);
  prefs.putUShort("qTo", cfg.quietTo);
  prefs.putUChar("qBright", cfg.quietBright);
  prefs.putBool("qMute", cfg.quietMute);
  prefs.putBool("qNoVibe", cfg.quietNoVibe);
  prefs.putUChar("qDisp", cfg.quietDisp);
  prefs.putFloat("lat", cfg.lat);
  prefs.putFloat("lon", cfg.lon);
  prefs.putBool("slWx", cfg.slWeather);
  prefs.putBool("slClk", cfg.slClock);
  prefs.putBool("slWifi", cfg.slWifi);
  prefs.putBool("slIp", cfg.slIp);
  prefs.putBool("slFw", cfg.slFw);
  prefs.putBool("lockInfo", cfg.lockInfo);
  prefs.putBool("sound", cfg.sound);
  prefs.putBool("vibe", cfg.vibe);
  prefs.putBool("pacSound", cfg.pacSound);
  prefs.putBool("shakeOn", cfg.shakeOn);
  prefs.putUChar("shakeSens", cfg.shakeSens);
  prefs.putUChar("cycMode", cfg.cycleMode);
  prefs.putUChar("cycOrder", cfg.cycleOrder);
  prefs.putUChar("cycSec", cfg.cycleSec);
  prefs.putUInt("emoMask", cfg.emoMask);
  prefs.putUChar("mqSpeed", cfg.mqSpeed);
  prefs.putUChar("sleepMin", cfg.sleepMin);
  prefs.end();
}

void factoryReset() {
  prefs.begin("pcbuddy", false);
  prefs.clear();
  prefs.end();
}

// =====================================================================
//  Ton & Vibration
// =====================================================================
bool soundOn() { return cfg.sound && !(quietNow && cfg.quietMute); }
bool vibeOn()  { return cfg.vibe && !(quietNow && cfg.quietNoVibe); }

void scheduleHeal(uint32_t inMs);

void beep(uint16_t f, uint16_t ms) {
  if (PIN_BUZZER < 0 || !soundOn() || f == 0) return;
  tone(PIN_BUZZER, f, ms);
  scheduleHeal(ms + 40);
}

void playNotes(const Note* n, uint8_t len) {
  if (PIN_BUZZER < 0 || !soundOn()) return;
  nqLen = min<uint8_t>(len, 10);
  for (uint8_t i = 0; i < nqLen; i++) nq[i] = n[i];
  nqPos = 0; nqNext = 0;
}

const Note SND_HAPPY[]  = {{1568, 60}, {2093, 90}};
const Note SND_LOVE[]   = {{1319, 90}, {1568, 90}, {2093, 160}};
const Note SND_NOTIFY[] = {{2093, 80}, {0, 60}, {2637, 120}};
const Note SND_WAKE[]   = {{1047, 60}, {1319, 60}, {1568, 80}};
const Note SND_BOOT[]   = {{1319, 70}, {1760, 110}};
const Note SND_ANGRY[]  = {{220, 90}, {175, 110}, {147, 220}};   // Knurren
const Note SND_UH[]     = {{494, 140}, {440, 140}, {392, 260}};   // "uhhh"

void soundLoop() {
  if (nqPos < nqLen && (int32_t)(millis() - nqNext) >= 0) {
    Note n = nq[nqPos++];
    if (n.f) { tone(PIN_BUZZER, n.f, n.ms); scheduleHeal(n.ms + 40); }
    nqNext = millis() + n.ms + 15;
  }
}

extern uint32_t vibeQuietUntil;
void vibrate(uint16_t ms) {
  if (PIN_VIBRO < 0 || !vibeOn()) return;
  digitalWrite(PIN_VIBRO, HIGH);
  vibeOff = millis() + ms;
  vibeQuietUntil = vibeOff + 300;                       // Motor schüttelt den Neigungssensor mit
}

void vibeLoop() {
  if (vibeOff && (int32_t)(millis() - vibeOff) >= 0) { digitalWrite(PIN_VIBRO, LOW); vibeOff = 0; scheduleHeal(40); }
}

// =====================================================================
//  Display, Helligkeit, Ruhemodus
// =====================================================================
uint8_t probeOled() {
  Wire.begin(PIN_SDA, PIN_SCL);
  const uint8_t cand[] = {OLED_ADDR, 0x3C, 0x3D};
  for (uint8_t a : cand) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) return a;
  }
  return 0;
}

uint8_t  oledAddr = OLED_ADDR;
bool     panelOff = false;          // Display per Power-Save aus (Ruhemodus "Display aus")
uint8_t  dimMask = 0;               // Software-Dimmung (nur 1,3"): 0 aus, 1 = 50 %, 2 = 25 % der Pixel
uint32_t healAt = 0, lastReinit = 0, lastI2CCheck = 0;

// Helligkeit in % setzen.
//  0,96" SSD1306: unter ~15 % zusätzlich Precharge/VCOMH absenken (funktioniert dort gut).
//  1,3"  SH1106 : KEINE Register-Eingriffe (die schalten ihn praktisch ab) – nur Kontrast und
//                 bei niedrigen Stufen Software-Dimmung (es leuchtet nur jedes 2./4. Pixel).
void setPanelLevel(int pct) {
  pct = constrain(pct, 1, 100);
  if (pct == panelLevel) return;
  panelLevel = pct;
  if (cfg.oled == 1) {
    dimMask = pct < 5 ? 2 : pct < 12 ? 1 : 0;
    d->setContrast(map(pct, 1, 100, 1, 255));
    return;
  }
  dimMask = 0;
  bool deep = pct <= 15;
  uint8_t contrast = deep ? map(pct, 1, 15, 0, 60) : map(pct, 16, 100, 20, 255);
  if (deep != deepDim) {
    deepDim = deep;
    d->sendF("ca", 0xD9, deep ? 0x11 : 0xF1);
    d->sendF("ca", 0xDB, deep ? 0x00 : 0x40);
  }
  d->setContrast(contrast);
}

void applyDimMask() {
  if (!dimMask) return;
  uint8_t* buf = d->getBufferPtr();
  static const uint8_t M25[4] = {0x11, 0x88, 0x44, 0x22};
  for (int x = 0; x < 128; x++) {
    uint8_t m = dimMask == 1 ? ((x & 1) ? 0xAA : 0x55) : M25[x & 3];
    for (int page = 0; page < 8; page++) buf[page * 128 + x] &= m;
  }
}

// ---- Selbstheilung (vor allem für 1,3"-SH1106) ----
// Ein Piepen oder der Vibrationsmotor kann die Versorgung kurz einbrechen lassen bzw. die
// I2C-Leitungen stören. Der SH1106 verliert dann seine Einstellungen und geht aus.
// Deshalb: nach Ton/Vibration und regelmäßig die Original-Initialisierung neu senden.
void scheduleHeal(uint32_t inMs) {
  uint32_t t = millis() + inMs;
  if (!healAt || (int32_t)(t - healAt) > 0) healAt = t;
}

void displayReinit() {
  d->initDisplay();                                   // komplette Init-Sequenz des Controllers
  d->setFlipMode(cfg.flip ? 1 : 0);
  d->setPowerSave(panelOff ? 1 : 0);
  int lv = panelLevel; panelLevel = -1; deepDim = false;
  setPanelLevel(lv);
  lastReinit = millis();
}

void displayHealth() {
  if (mode == M_UPDATE || !d) return;
  if (healAt && (int32_t)(millis() - healAt) >= 0) {   // nach Ton/Vibration
    healAt = 0;
    if (cfg.oled == 1) displayReinit();
  }
  if (since(lastI2CCheck) > 2000) {                    // antwortet das Display noch?
    lastI2CCheck = millis();
    Wire.beginTransmission(oledAddr);
    if (Wire.endTransmission() != 0) {
      Serial.println("[OLED] keine Antwort - I2C und Display neu starten");
      Wire.end();
      Wire.begin(PIN_SDA, PIN_SCL);
      d->begin();
      displayReinit();
    }
  }
  if (cfg.oled == 1 && since(lastReinit) > 15000) displayReinit();   // vorsorglich
}

void initDisplay() {
  uint8_t addr = probeOled();
  Serial.printf("[OLED] Typ: %s, I2C-Adresse: 0x%02X%s\n",
                cfg.oled ? "1,3\" SH1106" : "0,96\" SSD1306", addr ? addr : OLED_ADDR, addr ? "" : " (nicht gefunden)");
  if (!addr) addr = OLED_ADDR;
  oledAddr = addr;
  if (cfg.oled == 1) d = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);
  else               d = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);
  d->setI2CAddress(addr * 2);
  d->setBusClock(OLED_I2C_HZ);
  d->begin();
  d->setFlipMode(cfg.flip ? 1 : 0);
  d->setFontMode(1);
  setPanelLevel(cfg.bright);
}

bool quietScheduled() {
  if (!cfg.quietOn || !timeValid() || cfg.quietFrom == cfg.quietTo) return false;
  time_t t = time(nullptr); struct tm tm; localtime_r(&t, &tm);
  uint16_t m = tm.tm_hour * 60 + tm.tm_min;
  if (cfg.quietFrom < cfg.quietTo) return m >= cfg.quietFrom && m < cfg.quietTo;
  return m >= cfg.quietFrom || m < cfg.quietTo;   // über Mitternacht
}

void updateQuiet() {
  bool sched = quietScheduled();
  if (sched != quietLastSched) { quietLastSched = sched; quietOverride = -1; }  // Zeitplanwechsel hebt manuelle Wahl auf
  quietNow = quietOverride >= 0 ? (quietOverride == 1) : sched;
}

void updateBrightness() {
  int pct = quietNow ? cfg.quietBright : cfg.bright;
  if (mode == M_SLEEP) pct = min(pct, max(1, cfg.bright / 8));
  setPanelLevel(pct);
}

void drawCentered(const char* s, int y) {
  int w = d->getUTF8Width(s);
  d->drawUTF8((128 - w) / 2, y, s);
}

void showMsg(const String& a, const String& b, const String& c, uint32_t ms) {
  msgL1 = a; msgL2 = b; msgL3 = c;
  msgUntil = millis() + ms;
  msgReturn = (mode == M_INFO || (mode == M_CLOCK && !quietScreen)) ? mode : M_FACE;
  mode = M_MSG;
}

// ---- Grundformen ----
void drawHeart(float cx, float cy, float s) {          // s = halbe Breite
  int r = max(1, (int)(s * 0.5f + 0.5f));
  int x = (int)cx, y = (int)cy;
  d->drawDisc(x - r, y - r / 2, r);
  d->drawDisc(x + r, y - r / 2, r);
  d->drawTriangle(x - 2 * r, y - r / 2 + 1, x + 2 * r + 1, y - r / 2 + 1, x, y + 2 * r);
}

void drawSparkle(int x, int y, int s) {
  d->drawHLine(x - s, y, 2 * s + 1);
  d->drawVLine(x, y - s, 2 * s + 1);
}

void drawStar(float cx, float cy, float R, float rot) {   // gefüllter 5-zackiger Stern
  float px[10], py[10];
  for (int i = 0; i < 10; i++) {
    float a = rot - PI / 2 + i * PI / 5;
    float r = (i % 2) ? R * 0.45f : R;
    px[i] = cx + cosf(a) * r; py[i] = cy + sinf(a) * r;
  }
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    d->drawTriangle((int)cx, (int)cy, (int)px[i], (int)py[i], (int)px[j], (int)py[j]);
  }
}

void drawEyeBox(float cx, float cy, float w, float h) {
  int iw = max(2, (int)(w + 0.5f)), ih = max(2, (int)(h + 0.5f));
  int x = (int)(cx - iw / 2.0f), y = (int)(cy - ih / 2.0f);
  int r = min(8, (min(iw, ih) - 1) / 2);
  if (r < 1) d->drawBox(x, y, iw, ih);
  else d->drawRBox(x, y, iw, ih, r);
}

// =====================================================================
//  Gesicht / Emotionen
// =====================================================================
int parseEmotion(String n) {
  n.trim(); n.toLowerCase();
  for (int i = 0; i < E_COUNT; i++) if (n == EMO_NAMES[i]) return i;
  if (n == "kiss" || n == "kuss") return E_KISS;
  if (n == "calm" || n == "tired" || n == "muede") return E_SLEEPY;
  if (n == "happy" || n == "smile") return E_CUTE;
  if (n == "heart" || n == "hearts" || n == "minilove") return E_MINILOVE;
  if (n == "animelove") return E_ANIMELOVE;
  if (n == "cry") return E_CRYING;
  if (n == "herz" || n == "beat" || n == "pulse") return E_HEART;
  if (n == "wow" || n == "shock" || n == "surprise") return E_SURPRISED;
  if (n == "lachen" || n == "lol") return E_LAUGH;
  if (n == "stars" || n == "excited") return E_STAR;
  if (n == "huh" || n == "question") return E_CONFUSED;
  if (n == "woozy" || n == "queasy" || n == "uhh" || n == "wackelig" || n == "seasick") return E_UH;
  return -1;
}

void spawn(uint8_t type, float x, float y, float vx, float vy, int16_t life) {
  for (auto& p : parts) if (p.life <= 0) { p = {x, y, vx, vy, life, type}; return; }
}

void clearParticles() { for (auto& p : parts) p.life = 0; }

void setEmotion(Emotion e, uint32_t ms, bool activity = true) {
  emoReturn = M_FACE;
  cycleSet = false;
  quietScreen = false;
  emo = e;
  emoStart = millis();
  emoUntil = ms ? millis() + ms : 0;
  clearParticles();
  nextSpawn = 0;
  mode = M_FACE;
  if (activity) touchActivity();
}

bool emoSpecial(Emotion e) {   // Emotionen ohne normale Augenrechtecke
  return e == E_LOVE || e == E_ANIMELOVE || e == E_HEART || e == E_SURPRISED || e == E_DIZZY ||
         e == E_LAUGH || e == E_STAR;
}

void emoGeom(Emotion e, float& w, float& h, float& y) {
  w = 36; h = 36; y = 0;
  switch (e) {
    case E_SERIOUS:  w = 40; h = 28; y = 2; break;
    case E_SLEEPY:   w = 38; h = 8;  y = 8; break;
    case E_SAD:
    case E_CRYING:   w = 34; h = 30; y = 4; break;
    case E_ANGRY:    w = 38; h = 30; y = 4; break;
    case E_FIRE:     w = 36; h = 26; y = 9; break;
    case E_CUTE:
    case E_MINILOVE: w = 38; h = 30; y = 0; break;
    case E_KISS:     w = 36; h = 28; y = -2; break;
    case E_LEFT:
    case E_RIGHT:    w = 32; h = 34; y = 0; break;
    case E_CONFUSED: w = 34; h = 32; y = 2; break;
    case E_UH:       w = 36; h = 26; y = 2; break;
    default: break;
  }
}

float blinkScale(uint32_t now) {
  if (blinkStart == 0) return 1.0f;
  uint32_t t = now - blinkStart;
  if (t > 170) { blinkStart = 0; return 1.0f; }
  return max(0.06f, fabsf(cosf((float)t / 170.0f * PI)));
}

void updateFace(uint32_t now) {
  if (emoUntil && (int32_t)(now - emoUntil) > 0) {
    emo = E_NEUTRAL; emoUntil = 0; clearParticles();
    if (emoReturn != M_FACE) { mode = emoReturn; emoReturn = M_FACE; return; }   // z. B. zurück zur Uhr
  }

  float w, h, y;
  if (mode == M_SLEEP) { w = 38; h = 3; y = 10; }
  else emoGeom(emo, w, h, y);
  curW += (w - curW) * 0.3f;
  curH += (h - curH) * 0.3f;
  curY += (y - curY) * 0.3f;

  // Blinzeln
  bool canBlink = (mode == M_FACE) && emo != E_SLEEPY && !emoSpecial(emo);
  if (canBlink && (int32_t)(now - nextBlink) > 0) {
    blinkStart = now;
    nextBlink = now + random(2200, 6500);
    if (random(5) == 0) nextBlink = now + 260;   // Doppel-Blinzeln
  }

  // Blickrichtung / Kopfbewegung
  float t = now / 1000.0f;
  if (mode != M_FACE) { tgtX = 0; tgtY = 0; }
  else switch (emo) {
    case E_LEFT:     tgtX = -22; tgtY = 0; break;
    case E_RIGHT:    tgtX = 22;  tgtY = 0; break;
    case E_DIZZY:    tgtX = 7 * sinf(t * 4.0f); tgtY = 3 * cosf(t * 4.0f); break;
    case E_LAUGH:    tgtX = 0; tgtY = 2.5f * sinf(t * 14.0f); break;
    case E_CONFUSED: tgtX = 9 * sinf(t * 1.6f); tgtY = 0; break;
    case E_UH:       tgtX = 7 * sinf(t * 2.4f); tgtY = 2 + 1.5f * sinf(t * 4.8f); break;   // schwankt mit
    case E_SAD: case E_CRYING: tgtX = 0; tgtY = 3; break;
    case E_NEUTRAL: case E_CUTE:
      if ((int32_t)(now - nextGlance) > 0) {
        nextGlance = now + random(1400, 4800);
        if (random(3) == 0) { tgtX = 0; tgtY = 0; }
        else { tgtX = random(-14, 15); tgtY = random(-6, 7); }
      }
      break;
    default: tgtX = 0; tgtY = 0;
  }
  float k = (emo == E_DIZZY || emo == E_LAUGH) ? 0.5f : 0.25f;
  offX += (tgtX - offX) * k;
  offY += (tgtY - offY) * k;

  // Partikel erzeugen
  if ((int32_t)(now - nextSpawn) > 0) {
    nextSpawn = now + 500;
    if (mode == M_SLEEP || emo == E_SLEEPY) {
      spawn(1, 98 + offX, 22, 0.25f, -0.35f, 60); nextSpawn = now + 1100;
    } else if (mode == M_FACE) switch (emo) {
      case E_MINILOVE:  spawn(0, random(6, 122), 66, random(-3, 4) * 0.1f, -0.9f, 70); nextSpawn = now + 380; break;
      case E_KISS:      spawn(0, 70 + offX, 52, 0.45f, -0.6f, 55); nextSpawn = now + 900; break;
      case E_ANIMELOVE: spawn(2, random(4, 124), random(4, 60), 0, 0, 7); nextSpawn = now + 120; break;
      case E_STAR:      spawn(2, random(4, 124), random(2, 62), 0, 0, 6); nextSpawn = now + 160; break;
      case E_HEART:     spawn(0, random(2) ? random(4, 30) : random(98, 124), 66, 0, -0.7f, 80); nextSpawn = now + 650; break;
      default: break;
    }
  }
  for (auto& p : parts) if (p.life > 0) { p.x += p.vx; p.y += p.vy; p.life--; }
}

// Lidschnitt: inner=true -> zur Gesichtsmitte hin abfallend (wütend), sonst nach außen (traurig)
void cutSlant(float cx, float cy, float w, float h, bool leftEye, bool inner, float depth) {
  int x0 = (int)(cx - w / 2) - 1, x1 = (int)(cx + w / 2) + 1, top = (int)(cy - h / 2) - 1;
  int dep = (int)(h * depth);
  bool dropRight = (leftEye == inner);
  d->setDrawColor(0);
  if (dropRight) d->drawTriangle(x0, top, x1 + 1, top, x1 + 1, top + dep);
  else           d->drawTriangle(x0 - 1, top, x1, top, x0 - 1, top + dep);
  d->setDrawColor(1);
}

void cutHappy(float cx, float cy, float w, float h) {
  d->setDrawColor(0);
  d->drawFilledEllipse((int)cx, (int)(cy + h * 0.55f), (int)(w * 0.68f), (int)(h * 0.62f));
  d->setDrawColor(1);
}

void drawChevron(float cx, float cy) {     // "^"-Auge (geschlossen, fröhlich)
  for (int t = 0; t < 3; t++) {
    d->drawLine(cx - 13, cy + 3 + t, cx, cy - 4 + t);
    d->drawLine(cx, cy - 4 + t, cx + 13, cy + 3 + t);
  }
}

void drawTear(int x, int y) {
  d->drawDisc(x, y, 2);
  d->drawTriangle(x - 2, y, x + 3, y, x, y - 5);
}

void drawFlames(float cx, int base, uint32_t now, int seed) {
  for (int i = -1; i <= 1; i++) {
    float fh = 11 + 3 * sinf(now / 70.0f + i * 1.7f + seed) + (i == 0 ? 5 : 0);
    int bx = (int)cx + i * 10;
    int tip = bx + (int)(2.5f * sinf(now / 85.0f + i * 2 + seed));
    d->drawTriangle(bx - 7, base + 1, bx + 7, base + 1, tip, base - (int)fh);
    d->drawDisc(bx, base - 1, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  }
}

void drawSpiral(float cx, float cy, float rot) {
  float px = cx, py = cy;
  for (float a = 0.3f; a < 4.2f * PI; a += 0.28f) {
    float r = 1.0f + a * 0.95f;
    float x = cx + cosf(a + rot) * r, y = cy + sinf(a + rot) * r;
    d->drawLine((int)px, (int)py, (int)x, (int)y);
    d->drawLine((int)px + 1, (int)py, (int)x + 1, (int)y);
    px = x; py = y;
  }
}

// Pochendes Herz: Doppelschlag "ba-bumm"
float heartPulse(uint32_t now) {
  int t = (now - emoStart) % 900;
  auto p = [](int dt, int len) { return (dt < 0 || dt > len) ? 0.0f : sinf((float)dt / len * PI); };
  return 0.26f * p(t, 140) + 0.16f * p(t - 210, 150);
}

// Zeichnet Emotionen mit Sonderform. true = erledigt
bool drawSpecial(uint32_t now, float lx, float rx, float cy) {
  switch (emo) {
    case E_LOVE:
    case E_ANIMELOVE: {
      float s = (emo == E_ANIMELOVE) ? 15.0f * (1.0f + 0.16f * sinf(now / 110.0f))
                                     : 15.0f * (1.0f + 0.05f * sinf(now / 260.0f));
      drawHeart(lx, cy - 2, s);
      drawHeart(rx, cy - 2, s);
      return true;
    }
    case E_HEART: {
      float pulse = heartPulse(now);
      drawHeart(64 + offX * 0.3f, 28, 19.0f * (1.0f + pulse));
      if (pulse > 0.15f) {                       // Schlag-Linien
        for (int sgn = -1; sgn <= 1; sgn += 2) {
          d->drawLine(64 + sgn * 30, 14, 64 + sgn * 36, 9);
          d->drawLine(64 + sgn * 33, 26, 64 + sgn * 41, 25);
          d->drawLine(64 + sgn * 30, 38, 64 + sgn * 36, 43);
        }
      }
      return true;
    }
    case E_SURPRISED: {
      float pop = 1.0f - min(1.0f, since(emoStart) / 300.0f);
      int r = 13 + (int)(4 * pop);
      int ey = (int)cy - 2;
      d->drawDisc((int)lx, ey, r);
      d->drawDisc((int)rx, ey, r);
      d->setDrawColor(0);
      int jx = (int)(1.5f * sinf(now / 45.0f));
      d->drawDisc((int)lx + jx, ey, 4);
      d->drawDisc((int)rx + jx, ey, 4);
      d->setDrawColor(1);
      int mx = 64 + (int)offX;
      d->drawDisc(mx, 56, 5);
      d->setDrawColor(0); d->drawDisc(mx, 56, 3); d->setDrawColor(1);
      return true;
    }
    case E_DIZZY: {
      drawSpiral(lx, cy, now / 140.0f);
      drawSpiral(rx, cy, -(now / 140.0f) + PI);
      for (int i = 0; i < 3; i++) {             // kreisende Sternchen
        float a = now / 300.0f + i * 2.094f;
        drawSparkle(64 + (int)(34 * cosf(a)), 6 + (int)(3 * sinf(a)), 2);
      }
      return true;
    }
    case E_LAUGH: {
      drawChevron(lx, cy - 6);
      drawChevron(rx, cy - 6);
      int mx = 64 + (int)offX, my = 38 + (int)offY;
      d->drawFilledEllipse(mx, my, 16, 14, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
      d->setDrawColor(0);
      d->drawFilledEllipse(mx, my + 1, 13, 11, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
      d->setDrawColor(1);
      d->drawFilledEllipse(mx, my + 10, 6, 2);    // Zunge
      return true;
    }
    case E_STAR: {
      float R = 15.0f + 2.0f * sinf(now / 140.0f);
      drawStar(lx, cy - 1, R, now / 900.0f);
      drawStar(rx, cy - 1, R, -now / 900.0f);
      return true;
    }
    default: return false;
  }
}

void drawFace(uint32_t now) {
  const float GAP = 52;
  float lx = 64 - GAP / 2 + offX, rx = 64 + GAP / 2 + offX;
  float cy = 30 + offY + curY;
  float bl = blinkScale(now);

  if (!(mode == M_FACE && drawSpecial(now, lx, rx, cy))) {
    float w = curW, h = max(2.0f, curH * bl);
    float wl = w, wr = w, hl = h, hr = h, cyl = cy, cyr = cy;
    if (mode == M_FACE && emo == E_LEFT)  { wr = w * 0.82f; hr = h * 0.9f; }
    if (mode == M_FACE && emo == E_RIGHT) { wl = w * 0.82f; hl = h * 0.9f; }
    if (mode == M_FACE && emo == E_CONFUSED) { wr = w * 0.7f; hr = h * 0.62f; cyr = cy - 5; }

    if (mode == M_FACE && emo == E_KISS) {
      drawChevron(lx, cy);
      drawEyeBox(rx, cy, wr, hr);
      if (bl > 0.5f) cutHappy(rx, cy, wr, hr);
    } else {
      drawEyeBox(lx, cyl, wl, hl);
      drawEyeBox(rx, cyr, wr, hr);
    }

    if (mode == M_FACE && bl > 0.5f) {
      switch (emo) {
        case E_ANGRY:
        case E_FIRE:
          cutSlant(lx, cy, wl, hl, true, true, 0.6f);
          cutSlant(rx, cy, wr, hr, false, true, 0.6f);
          break;
        case E_SAD:
        case E_CRYING:
          cutSlant(lx, cy, wl, hl, true, false, 0.5f);
          cutSlant(rx, cy, wr, hr, false, false, 0.5f);
          break;
        case E_SERIOUS:
          d->setDrawColor(0);
          d->drawBox((int)(lx - wl / 2) - 1, (int)(cy - hl / 2) - 1, (int)wl + 2, (int)(hl * 0.32f) + 1);
          d->drawBox((int)(rx - wr / 2) - 1, (int)(cy - hr / 2) - 1, (int)wr + 2, (int)(hr * 0.32f) + 1);
          d->setDrawColor(1);
          break;
        case E_CUTE:
        case E_MINILOVE:
          cutHappy(lx, cy, wl, hl);
          cutHappy(rx, cy, wr, hr);
          break;
        case E_CONFUSED:
          cutSlant(lx, cyl, wl, hl, true, false, 0.35f);
          break;
        default: break;
      }
    }

    if (mode == M_FACE) {
      if (emo == E_CUTE) {
        for (int i = 0; i < 3; i++) {
          d->drawLine(lx - 16 + i * 5, cy + 16, lx - 13 + i * 5, cy + 12);
          d->drawLine(rx + 6 + i * 5, cy + 16, rx + 9 + i * 5, cy + 12);
        }
      }
      if (emo == E_CRYING) {
        for (int k = 0; k < 2; k++) {
          int ph = (int)((now / 14 + k * 17) % 34);
          drawTear((int)(lx - wl / 2 + 5), (int)(cy + hl / 2) + 2 + ph);
          drawTear((int)(rx + wr / 2 - 5), (int)(cy + hr / 2) + 2 + ((ph + 11) % 34));
        }
      }
      if (emo == E_FIRE) {
        drawFlames(lx, (int)(cy - hl / 2) - 2, now, 0);
        drawFlames(rx, (int)(cy - hr / 2) - 2, now, 3);
      }
      if (emo == E_KISS) {
        int mx = 64 + (int)offX;
        d->drawCircle(mx, 51, 3, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
        d->drawCircle(mx, 57, 3, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
      }
      if (emo == E_UH) {                              // "Uh": schwere Lider, kleiner Mund, Schweißtropfen
        float tilt = sinf(now / 1000.0f * 2.4f);
        int lidL = (int)(hl * (0.42f + 0.12f * tilt)), lidR = (int)(hr * (0.42f - 0.12f * tilt));
        d->setDrawColor(0);
        d->drawBox((int)(lx - wl / 2) - 1, (int)(cy - hl / 2) - 1, (int)wl + 2, lidL + 1);
        d->drawBox((int)(rx - wr / 2) - 1, (int)(cy - hr / 2) - 1, (int)wr + 2, lidR + 1);
        d->setDrawColor(1);
        int mx = 64 + (int)offX, my = 53 + (int)(offY * 0.5f);
        d->drawEllipse(mx, my, 7, 4); d->drawEllipse(mx, my, 6, 3);
        int sx = min(122, (int)(rx + wr / 2) + 5);
        drawTear(sx, (int)(cy - hr / 2) + 2 + (int)((now / 120) % 4));
      }
      if (emo == E_CONFUSED) {
        d->setFont(u8g2_font_helvB14_tf);
        int qy = 16 + (int)(2 * sinf(now / 250.0f));
        d->drawStr(min(118, (int)(rx + wr / 2) + 4), qy, "?");
        int mx = 64 + (int)offX;                      // Wellen-Mund
        for (int x = -10; x < 10; x++) {
          int y1 = 55 + (int)(2 * sinf(x * 0.6f)), y2 = 55 + (int)(2 * sinf((x + 1) * 0.6f));
          d->drawLine(mx + x, y1, mx + x + 1, y2);
        }
      }
    }
  }

  d->setFont(u8g2_font_6x10_tf);
  for (auto& p : parts) {
    if (p.life <= 0) continue;
    if (p.type == 0) drawHeart(p.x, p.y, 4);
    else if (p.type == 1) d->drawStr((int)p.x, (int)p.y, p.life > 30 ? "z" : "Z");
    else drawSparkle((int)p.x, (int)p.y, p.life > 3 ? 3 : 1);
  }
}

// =====================================================================
//  Wetter
// =====================================================================
float jnum(const String& s, const char* key) {
  String k = String("\"") + key + "\":";
  int p = s.indexOf(k);
  if (p < 0) return NAN;
  return s.substring(p + k.length(), p + k.length() + 12).toFloat();
}

void fetchWeather() {
  WiFiClientSecure cl;
  cl.setInsecure();
  HTTPClient http;
  http.setTimeout(9000);
  char url[220];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&timezone=auto",
           cfg.lat, cfg.lon);
  if (!http.begin(cl, url)) return;
  int code = http.GET();
  if (code == 200) {
    String b = http.getString();
    int p = b.indexOf("\"current\":{");
    if (p >= 0) {
      String c = b.substring(p);
      float t = jnum(c, "temperature_2m");
      if (!isnan(t)) {
        wxTemp = t;
        wxHum  = (int)jnum(c, "relative_humidity_2m");
        wxCode = (int)jnum(c, "weather_code");
        wxWind = jnum(c, "wind_speed_10m");
        wxValid = true;
        Serial.printf("[Wetter] %.1f C, Code %d\n", t, wxCode);
      }
    }
  } else {
    Serial.printf("[Wetter] HTTP-Fehler %d\n", code);
  }
  http.end();
}

extern volatile bool wifiForce;
extern volatile uint8_t dnsFails;

void weatherTask(void*) {
  uint32_t lastDns = 0;
  for (;;) {
    uint32_t now = millis();
    // Lebt die Verbindung wirklich? Alle 2 min DNS testen; 3 Fehler in Folge -> neu verbinden
    if (wifiOk() && now - lastDns > 120000) {
      lastDns = now;
      IPAddress ip;
      if (WiFi.hostByName("pool.ntp.org", ip) == 1 && ip != IPAddress(0, 0, 0, 0)) dnsFails = 0;
      else { dnsFails = dnsFails + 1; if (dnsFails >= 3) { dnsFails = 0; wifiForce = true; } }
    }
    bool due = wxForce || (now - wxLast > WEATHER_INTERVAL_MS) || (!wxValid && now - wxLast > 60000);
    if (wifiOk() && due) {
      wxForce = false;
      fetchWeather();
      wxLast = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

const char* wxText(int c) {
  if (c < 0)  return "-";
  if (c == 0) return "Klar";
  if (c <= 2) return "Heiter";
  if (c == 3) return "Bewölkt";
  if (c == 45 || c == 48) return "Nebel";
  if (c >= 51 && c <= 57) return "Niesel";
  if (c >= 61 && c <= 67) return "Regen";
  if (c >= 71 && c <= 77) return "Schnee";
  if (c >= 80 && c <= 82) return "Schauer";
  if (c == 85 || c == 86) return "Schneeschauer";
  if (c >= 95) return "Gewitter";
  return "?";
}

void drawSun(int cx, int cy, int r) {
  d->drawDisc(cx, cy, r);
  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4;
    d->drawLine(cx + cosf(a) * (r + 3), cy + sinf(a) * (r + 3), cx + cosf(a) * (r + 6), cy + sinf(a) * (r + 6));
  }
}

void drawCloud(int ox, int oy) {
  const int dx[3] = {12, 22, 31}, dy[3] = {20, 15, 21}, rr[3] = {7, 9, 6};
  d->setDrawColor(0);
  for (int i = 0; i < 3; i++) d->drawDisc(ox + dx[i], oy + dy[i], rr[i] + 2);
  d->drawBox(ox + 10, oy + 18, 23, 12);
  d->setDrawColor(1);
  for (int i = 0; i < 3; i++) d->drawDisc(ox + dx[i], oy + dy[i], rr[i]);
  d->drawBox(ox + 12, oy + 20, 19, 8);
}

void drawWxIcon(int c, int ox, int oy) {
  if (c == 0) { drawSun(ox + 20, oy + 18, 8); return; }
  if (c <= 2) { drawSun(ox + 14, oy + 12, 6); drawCloud(ox + 2, oy + 4); return; }
  if (c == 45 || c == 48) { for (int i = 0; i < 4; i++) d->drawHLine(ox + 4 + (i % 2) * 4, oy + 10 + i * 6, 30); return; }
  drawCloud(ox, oy);
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82))
    for (int i = 0; i < 4; i++) d->drawLine(ox + 13 + i * 6, oy + 31, ox + 10 + i * 6, oy + 36);
  else if ((c >= 71 && c <= 77) || c == 85 || c == 86)
    for (int i = 0; i < 4; i++) d->drawDisc(ox + 12 + i * 6, oy + 32 + (i % 2) * 3, 1);
  else if (c >= 95) {
    d->drawLine(ox + 22, oy + 28, ox + 17, oy + 34); d->drawLine(ox + 17, oy + 34, ox + 23, oy + 34);
    d->drawLine(ox + 23, oy + 34, ox + 18, oy + 40);
  }
}

// =====================================================================
//  Pac-Man-Uhr
//  Ziffern aus Pellets. Pac-Man (verfolgt von einem Geist) läuft auf einem
//  Rundkurs ober- und unterhalb der Ziffern – ohne die Pellets der Uhrzeit zu
//  berühren. Erst kurz vor dem Minutenwechsel geht er genau in die Ziffern,
//  die sich ändern, frisst sie, und zur vollen Minute erscheint die neue Zahl.
// =====================================================================
const uint8_t DIGIT5x7[10][7] = {
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  // 0
  {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 1
  {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},  // 2
  {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},  // 3
  {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},  // 4
  {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},  // 5
  {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},  // 6
  {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // 7
  {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  // 8
  {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},  // 9
};
const int PC_DOT = 3, PC_PITCH = 5, PC_DY = 16;     // Ziffern: y 16..48
#define PC_W (4 * PC_PITCH + PC_DOT)                // 23 px
const int PC_DX[4] = {64 - 2 * PC_W - 15, 64 - PC_W - 11, 64 + 11, 64 + PC_W + 15};   // 3, 30, 75, 102
const int PAC_R = 5;                                 // Figurenradius
const int LANE_TOP = 7, LANE_BOT = 57;               // Rundkurs (berührt keine Ziffer)
const int EAT_Y[3] = {22, 34, 46};                   // Fressbahnen innerhalb einer Ziffer
const int OFF_L = -12, OFF_R = 140;                  // Wendepunkte außerhalb des Bildes
#define PAC_ROAM_SPEED 2                             // px pro 30-ms-Schritt
#define PAC_FAST_SPEED 4

PacState pacState = PAC_ROAM;
PacWP    pacPath[40];
uint8_t  pacPathLen = 0, pacPathIdx = 0;
float    pacX = OFF_L, pacY = LANE_TOP;
int8_t   pacDX = 1, pacDY = 0;
int8_t   ringCorner = 0, ringDir = 1;               // Ecken: 0 oben-links, 1 oben-rechts, 2 unten-rechts, 3 unten-links
const int16_t RING_X[4] = {OFF_L, OFF_R, OFF_R, OFF_L};
const int16_t RING_Y[4] = {LANE_TOP, LANE_TOP, LANE_BOT, LANE_BOT};
uint8_t  pcPop = 7;
bool     pcEaten = false;
int8_t   planCorner = 0, planDir = 1;
bool     pcWakaNow = false;          // Waka-Waka nur bei Stundenwechsel
int16_t  pacHX[16], pacHY[16];
int8_t   pacHDX[16];
uint8_t  pacHPos = 0;
bool     pcHistInit = false;

void pcFill(int i, int8_t digit, uint8_t rows) {
  for (int r = 0; r < 7; r++) pcMask[i][r] = (r < rows && digit >= 0) ? DIGIT5x7[digit][r] : 0;
}

void pcDigitsAt(time_t t, int8_t out[4]) {
  struct tm tm; localtime_r(&t, &tm);
  out[0] = tm.tm_hour / 10; out[1] = tm.tm_hour % 10; out[2] = tm.tm_min / 10; out[3] = tm.tm_min % 10;
}

void pcCurrent(int8_t out[4]) { pcDigitsAt(time(nullptr), out); }

bool pcDiff(const int8_t a[4], const int8_t b[4]) {
  for (int i = 0; i < 4; i++) if (a[i] != b[i]) return true;
  return false;
}

void pacRoamFromCorner();

void pcReset() {
  int8_t cur[4]; pcCurrent(cur);
  for (int i = 0; i < 4; i++) { pcShown[i] = cur[i]; pcFill(i, cur[i], 7); }
  pcPop = 7; pcWait = false; pcEaten = false;
  if (pacState == PAC_EAT) { pacState = PAC_ROAM; pacX = OFF_L; pacY = LANE_TOP; ringCorner = 0; pacPathLen = 0; }
}

// Demo: alle vier Ziffern werden sofort angesteuert und gefressen
void pcDemo() {
  for (int i = 0; i < 4; i++) { pcShown[i] = (pcShown[i] < 0 ? 0 : pcShown[i] + 3) % 10; pcFill(i, pcShown[i], 7); }
  pcPop = 7; pcWait = false; pcEaten = false;
  pcLastDraw = millis();
}

// ---- Rundkurs ----
void pacRoamFromCorner() {
  if (random(100) < 30) ringDir = -ringDir;                 // an einer (unsichtbaren) Ecke umdrehen
  int next = (ringCorner + ringDir + 4) % 4;
  pacPath[0] = {RING_X[next], RING_Y[next], false};
  pacPathLen = 1; pacPathIdx = 0;
  ringCorner = next;
}

// ---- Fressplan: nur in die geänderten Ziffern, Stunden und Minuten getrennt ----
uint8_t pcPlanAdd(PacWP* p, uint8_t n, int x, int y, bool eat) {
  if (n < 40 && (n == 0 || p[n - 1].x != x || p[n - 1].y != y)) p[n++] = {(int16_t)x, (int16_t)y, eat};
  return n;
}

// Baut den Weg ab der aktuellen Position, liefert die Dauer in ms
uint32_t pcBuildPlan(const int8_t target[4], PacWP* p, uint8_t& n) {
  n = 0;
  for (int i = 0; i < 4; i++) { pcChanged[i] = target[i] != pcShown[i]; pcNew[i] = target[i]; }
  int gx0[2], gx1[2], gc = 0;
  for (int side = 0; side < 2; side++) {                     // 0 = Stunden, 1 = Minuten
    int a = side * 2, first = -1, last = -1;
    for (int i = a; i < a + 2; i++) if (pcChanged[i]) { if (first < 0) first = i; last = i; }
    if (first < 0) continue;
    gx0[gc] = PC_DX[first] + 2;                              // Mittelpunkt-Bereich: Figur bleibt innerhalb der Gruppe
    gx1[gc] = PC_DX[last] + PC_W - 2;
    gc++;
  }
  if (!gc) return 0;
  int x = (int)pacX, y = (int)pacY;
  bool fromTop = y < 32;
  int lane = fromTop ? LANE_TOP : LANE_BOT;
  n = pcPlanAdd(p, n, x, lane, false);                       // ggf. außerhalb des Bildes auf die Bahn
  int order[2] = {0, 1};
  if (gc == 2 && abs(x - (gx0[1] + gx1[1]) / 2) < abs(x - (gx0[0] + gx1[0]) / 2)) { order[0] = 1; order[1] = 0; }
  for (int k = 0; k < gc; k++) {
    int g = order[k];
    int sx = abs(x - gx0[g]) <= abs(x - gx1[g]) ? gx0[g] : gx1[g];
    int ex = sx == gx0[g] ? gx1[g] : gx0[g];
    int L[3] = {EAT_Y[0], EAT_Y[1], EAT_Y[2]};
    if (!fromTop) { L[0] = EAT_Y[2]; L[2] = EAT_Y[0]; }
    n = pcPlanAdd(p, n, sx, lane, false);                    // auf der Bahn bis über die Ziffer
    n = pcPlanAdd(p, n, sx, L[0], true);                     // hinein
    n = pcPlanAdd(p, n, ex, L[0], true);
    n = pcPlanAdd(p, n, ex, L[1], true);
    n = pcPlanAdd(p, n, sx, L[1], true);
    n = pcPlanAdd(p, n, sx, L[2], true);
    n = pcPlanAdd(p, n, ex, L[2], true);
    fromTop = !fromTop;                                      // auf der anderen Seite wieder hinaus
    lane = fromTop ? LANE_TOP : LANE_BOT;
    n = pcPlanAdd(p, n, ex, lane, false);
    x = ex;
  }
  // danach weiter im Rundkurs: zur nächsten Ecke auf dieser Bahn
  bool goRight = x >= 64;
  int corner = fromTop ? (goRight ? 1 : 0) : (goRight ? 2 : 3);
  n = pcPlanAdd(p, n, RING_X[corner], RING_Y[corner], false);
  planCorner = corner;
  planDir = fromTop ? (goRight ? 1 : -1) : (goRight ? -1 : 1);
  // Dauer bis zum letzten Fress-Punkt
  int lastEat = -1;
  for (uint8_t i = 0; i < n; i++) if (p[i].eat) lastEat = i;
  uint32_t steps = 0;
  float cx = pacX, cy = pacY;
  for (int i = 0; i <= lastEat; i++) {
    steps += (uint32_t)((fabsf(p[i].x - cx) + fabsf(p[i].y - cy)) / PAC_FAST_SPEED + 0.99f);
    cx = p[i].x; cy = p[i].y;
  }
  return steps * 30;
}

void pcAdoptPlan(const PacWP* p, uint8_t n, bool waitForChange) {
  memcpy(pacPath, p, sizeof(PacWP) * n);
  pacPathLen = n; pacPathIdx = 0;
  ringCorner = planCorner; ringDir = planDir;
  pacState = PAC_EAT;
  pcWait = waitForChange;
  pcEaten = false;
  pcWakaNow = cfg.pacSound && (pcChanged[0] || pcChanged[1]);   // nur wenn sich die Stunde ändert
}

void pacRecord() {
  pacHPos = (pacHPos + 1) % 16;
  pacHX[pacHPos] = (int16_t)pacX; pacHY[pacHPos] = (int16_t)pacY; pacHDX[pacHPos] = pacDX ? pacDX : pacHDX[(pacHPos + 15) % 16];
}

void pacEatAround() {
  bool ate = false;
  for (int i = 0; i < 4; i++) {
    if (!pcChanged[i]) continue;                             // nur Ziffern, die sich ändern
    for (int r = 0; r < 7; r++) for (int c = 0; c < 5; c++) {
      uint8_t bit = 0x10 >> c;
      if (!(pcMask[i][r] & bit)) continue;
      float dx = PC_DX[i] + c * PC_PITCH + PC_DOT / 2.0f - pacX;
      float dy = PC_DY + r * PC_PITCH + PC_DOT / 2.0f - pacY;
      if (fabsf(dx) <= 4.5f && fabsf(dy) <= 7) { pcMask[i][r] &= ~bit; ate = true; }
    }
  }
  if (ate && pcWakaNow && since(pcLastWaka) > 110) {
    pcLastWaka = millis(); pcWakaHi = !pcWakaHi;
    beep(pcWakaHi ? 660 : 440, 45);
  }
}

void pcStep() {
  if (pacPathIdx >= pacPathLen) {                            // Weg zu Ende
    if (pacState == PAC_EAT) pacState = PAC_ROAM;
    pacRoamFromCorner();
  }
  PacWP& w = pacPath[pacPathIdx];
  float sp = pacState == PAC_EAT ? PAC_FAST_SPEED : PAC_ROAM_SPEED;
  float dx = w.x - pacX, dy = w.y - pacY;
  if (fabsf(dx) > 0.01f) { pacDX = dx > 0 ? 1 : -1; pacDY = 0; pacX += fabsf(dx) < sp ? dx : pacDX * sp; }
  else if (fabsf(dy) > 0.01f) { pacDY = dy > 0 ? 1 : -1; pacDX = 0; pacY += fabsf(dy) < sp ? dy : pacDY * sp; }
  if (w.eat) pacEatAround();
  if (fabsf(w.x - pacX) < 0.01f && fabsf(w.y - pacY) < 0.01f) pacPathIdx++;
  pacRecord();

  // alles gefressen? -> neue Ziffern vormerken
  if (pacState == PAC_EAT && !pcEaten) {
    bool empty = true;
    for (int i = 0; i < 4; i++) if (pcChanged[i]) for (int r = 0; r < 7; r++) if (pcMask[i][r]) empty = false;
    if (empty) {
      pcEaten = true;
      for (int i = 0; i < 4; i++) if (pcChanged[i]) pcShown[i] = pcNew[i];
      pcPop = 0;
      pcWaitSince = millis();
    }
  }
  // neue Ziffern aufbauen – bei vorgezogenem Fressen erst, wenn die Minute gewechselt hat
  if (pcEaten && pcPop < 7) {
    if (pcWait && pcDiff(pcNow, pcNew) && since(pcWaitSince) < 8000) return;
    pcWait = false;
    pcPop++;
    for (int i = 0; i < 4; i++) if (pcChanged[i]) pcFill(i, pcShown[i], pcPop);
    if (pcPop == 7) { pcEaten = false; if (pcWakaNow) beep(1760, 40); pcWakaNow = false; }
  }
}

// mouth: 0 = zu, 1 = halb, 2 = weit offen; (dx,dy) = Blickrichtung
void drawPacman(int x, int y, int r, int dx, int dy, uint8_t mouth) {
  d->drawDisc(x, y, r);
  if (!mouth) return;
  int h = mouth == 2 ? r + 3 : r / 2 + 1;
  d->setDrawColor(0);
  if (dy == 0) d->drawTriangle(x - dx, y, x + dx * (r + 2), y - h, x + dx * (r + 2), y + h);
  else         d->drawTriangle(x, y - dy, x - h, y + dy * (r + 2), x + h, y + dy * (r + 2));
  d->setDrawColor(1);
}

uint8_t pacMouth(uint32_t now) { static const uint8_t P[4] = {2, 1, 0, 1}; return P[(now / 70) % 4]; }

void drawGhost(int x, int y, int dir, uint32_t now, int r) {   // (x,y) = Mitte
  d->drawDisc(x, y - 1, r, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  d->drawBox(x - r, y - 1, 2 * r + 1, r + 1);
  bool ph = (now / 150) % 2;                              // wackelnder Saum
  for (int i = 0; i <= 2 * r; i++) if ((i + ph) % 3 != 1) d->drawPixel(x - r + i, y + r);
  int ew = r > 4 ? 3 : 2, eh = r > 4 ? 4 : 3, ex = r > 4 ? 4 : 3;
  d->setDrawColor(0);
  d->drawBox(x - ex, y - eh + 1 - (r > 4), ew, eh); d->drawBox(x + ex - ew + 1, y - eh + 1 - (r > 4), ew, eh);
  d->setDrawColor(1);
  int px = dir > 0 ? ew - 1 : 0;                          // Pupillen schauen in Laufrichtung
  d->drawPixel(x - ex + px, y - 1 - (r > 4)); d->drawPixel(x + ex - ew + 1 + px, y - 1 - (r > 4));
}

void drawPacClock(uint32_t now, int ox, int oy) {
  if (!timeValid()) {
    d->setFont(u8g2_font_pressstart2p_8f);
    drawCentered("READY!", 30);
    d->setFont(u8g2_font_helvR08_tf);
    drawCentered("Zeit wird geholt...", 46);
    return;
  }
  if (since(pcLastDraw) > 2000 || pcShown[0] < 0) pcReset();
  pcLastDraw = millis();
  if (!pcHistInit) {
    for (int i = 0; i < 16; i++) { pacHX[i] = (int16_t)pacX; pacHY[i] = (int16_t)pacY; pacHDX[i] = 1; }
    pcHistInit = true;
  }

  struct timeval tv; gettimeofday(&tv, nullptr);
  struct tm tm; localtime_r(&tv.tv_sec, &tm);
  pcDigitsAt(tv.tv_sec, pcNow);

  if (pacState == PAC_ROAM && pcPop >= 7) {
    static PacWP plan[40];
    uint8_t n = 0;
    int32_t msToNext = (59 - tm.tm_sec) * 1000 + (999 - tv.tv_usec / 1000);
    int8_t nxt[4]; pcDigitsAt(tv.tv_sec - tm.tm_sec + 60, nxt);
    if (pcDiff(pcNow, pcShown)) {                          // Zeitsprung / Demo: sofort hin und fressen
      if (pcBuildPlan(pcNow, plan, n)) pcAdoptPlan(plan, n, false);
    } else if (pcDiff(nxt, pcShown) && msToNext < 10000) {
      uint32_t dur = pcBuildPlan(nxt, plan, n);
      if (dur && msToNext <= (int32_t)(dur + 350)) pcAdoptPlan(plan, n, true);   // gerade rechtzeitig losrennen
    }
  }
  uint8_t steps = 0;
  while (since(pcLastStep) >= 30 && steps++ < 4) { pcLastStep += 30; pcStep(); }
  if (steps >= 4) pcLastStep = millis();

  // Ziffern aus Pellets
  for (int i = 0; i < 4; i++)
    for (int r = 0; r < 7; r++)
      for (int c = 0; c < 5; c++)
        if (pcMask[i][r] & (0x10 >> c))
          d->drawBox(ox + PC_DX[i] + c * PC_PITCH, oy + PC_DY + r * PC_PITCH, PC_DOT, PC_DOT);

  // Doppelpunkt = Power-Pellets (blinken)
  if (tm.tm_sec % 2 == 0) {
    d->drawDisc(ox + 64, oy + PC_DY + 9, 2);
    d->drawDisc(ox + 64, oy + PC_DY + 24, 2);
  }

  // Geist folgt Pac-Mans Spur
  uint8_t gi = (pacHPos + 16 - (pacState == PAC_ROAM ? 9 : 5)) % 16;
  drawGhost(ox + pacHX[gi], oy + pacHY[gi], pacHDX[gi] ? pacHDX[gi] : 1, now, PAC_R);
  drawPacman(ox + (int)pacX, oy + (int)pacY, PAC_R, pacDX, pacDY, pacMouth(now));
}

// =====================================================================
//  Info-Folien
// =====================================================================
void buildSlides() {
  slideCount = 0;
  if (cfg.slWeather) slides[slideCount++] = S_WEATHER;
  if (cfg.slClock)   slides[slideCount++] = S_CLOCK;
  if (cfg.slWifi)    slides[slideCount++] = S_WIFI;
  if (cfg.slIp)      slides[slideCount++] = S_IP;
  if (cfg.slFw || slideCount == 0) slides[slideCount++] = S_FW;
}

void enterInfo() {
  buildSlides();
  slideIdx = 0;
  mode = M_INFO;
  quietScreen = false;
  touchActivity();
}

void slideHeader(const char* title) {
  d->setFont(u8g2_font_helvB08_tf);
  d->drawUTF8(0, 8, title);
  if (timeValid()) {
    char b[8]; time_t t = time(nullptr); struct tm tm; localtime_r(&t, &tm);
    strftime(b, sizeof(b), "%H:%M", &tm);
    d->setFont(u8g2_font_helvR08_tf);
    d->drawStr(128 - d->getStrWidth(b), 8, b);
  }
  d->drawHLine(0, 11, 128);
  int x0 = 64 - (slideCount - 1) * 4;
  for (int i = 0; i < slideCount; i++) {
    if (i == slideIdx) d->drawDisc(x0 + i * 8, 62, 1);
    else d->drawPixel(x0 + i * 8, 62);
  }
}

void drawInfo(uint32_t now) {
  char b[48];
  switch (slides[slideIdx]) {
    case S_WEATHER: {
      String t = "Wetter · " + cfg.city;
      slideHeader(t.c_str());
      if (!wxValid) {
        d->setFont(u8g2_font_helvR10_tf);
        drawCentered(wifiOk() ? "Lade Wetter..." : "Kein WLAN", 40);
        break;
      }
      drawWxIcon(wxCode, 2, 15);
      d->setFont(u8g2_font_logisoso24_tf);
      snprintf(b, sizeof(b), "%.0f°", (float)wxTemp);
      d->drawUTF8(50, 42, b);
      d->setFont(u8g2_font_helvR08_tf);
      d->drawUTF8(50, 55, wxText(wxCode));
      snprintf(b, sizeof(b), "%d%%", (int)wxHum);
      d->drawStr(128 - d->getStrWidth(b), 55, b);
      break;
    }
    case S_CLOCK:
      drawPacClock(now, 0, 0);
      break;
    case S_WIFI: {
      slideHeader("WLAN");
      d->setFont(u8g2_font_helvB10_tf);
      if (!wifiOk()) { drawCentered("Nicht verbunden", 34); d->setFont(u8g2_font_helvR08_tf); drawCentered("5 s halten = Setup", 50); break; }
      drawCentered(WiFi.SSID().c_str(), 30);
      int rssi = WiFi.RSSI();
      int lv = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -78 ? 2 : 1;
      for (int i = 0; i < 4; i++) {
        int bh = 4 + i * 3;
        if (i < lv) d->drawBox(44 + i * 6, 54 - bh, 4, bh); else d->drawFrame(44 + i * 6, 54 - bh, 4, bh);
      }
      d->setFont(u8g2_font_helvR08_tf);
      snprintf(b, sizeof(b), "%d dBm", rssi);
      d->drawStr(72, 54, b);
      break;
    }
    case S_IP: {
      slideHeader("Adresse");
      d->setFont(u8g2_font_helvB10_tf);
      String ip = wifiOk() ? WiFi.localIP().toString() : String("-");
      drawCentered(ip.c_str(), 32);
      d->setFont(u8g2_font_helvR08_tf);
      String h = cfg.host + ".local";
      drawCentered(h.c_str(), 48);
      break;
    }
    case S_FW: {
      slideHeader("Firmware");
      d->setFont(u8g2_font_helvB10_tf);
      drawCentered(DEVICE_NAME " v" FW_VERSION, 28);
      d->setFont(u8g2_font_helvR08_tf);
      uint32_t s = millis() / 1000;
      snprintf(b, sizeof(b), "Laufzeit %luh %02lum", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60));
      drawCentered(b, 42);
      snprintf(b, sizeof(b), "%s · %u kB frei", cfg.oled ? "SH1106" : "SSD1306", (unsigned)(ESP.getFreeHeap() / 1024));
      drawCentered(b, 54);
      break;
    }
  }
}

// =====================================================================
//  Lauftext & stehender Text
// =====================================================================
// Schrift mit Umlauten, ß, €, °, µ, §, ² (Latin-1 + Latin Extended)
#define MQ_FONT u8g2_font_helvB18_te

float mqPxPerSec(uint8_t s) { return 20.0f + constrain(s, 1, 10) * 22.0f; }   // 1 = 42 px/s ... 10 = 240 px/s

void startMarquee(const String& text, bool loop) {
  mqText = normalizeText(text);
  if (mqText.length() > 200) mqText = mqText.substring(0, 200);
  mqLoop = loop;
  d->setFont(MQ_FONT);
  mqW = d->getUTF8Width(mqText.c_str());
  mqPos = 128;
  mqLast = millis();
  mode = M_MARQUEE;
  quietScreen = false;
  touchActivity();
}

void marqueeLoop() {
  uint32_t dt = since(mqLast);
  if (dt < 10) return;
  mqLast += dt;
  if (dt > 100) dt = 100;
  mqPos -= mqPxPerSec(cfg.mqSpeed) * dt / 1000.0f;
  if (mqPos < -mqW) {
    if (mqLoop) mqPos = 128;
    else setEmotion(E_CUTE, 3000);
  }
}

void drawMarquee() {
  d->setFont(MQ_FONT);
  d->drawUTF8((int)mqPos, 41, mqText.c_str());
}

const uint8_t* const TXT_FONTS[] = {u8g2_font_helvB18_te, u8g2_font_helvB14_te, u8g2_font_helvB12_te, u8g2_font_helvB10_te, u8g2_font_helvB08_te};
const uint8_t TXT_LH[] = {22, 17, 15, 13, 10};

uint8_t utf8Len(uint8_t c) { return c < 0x80 ? 1 : c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1; }

bool wrapInto(const String& s, uint8_t fi, uint8_t& count) {
  d->setFont(TXT_FONTS[fi]);
  uint8_t maxLines = min<uint8_t>(64 / TXT_LH[fi], TXT_MAX_LINES);
  const int W = 126;
  count = 0;
  String line;
  size_t i = 0;
  while (i <= s.length()) {
    int sp = s.indexOf(' ', i);
    if (sp < 0) sp = s.length();
    String word = s.substring(i, sp);
    i = sp + 1;
    if (!word.length()) { if ((size_t)sp >= s.length()) break; continue; }
    String cand = line.length() ? line + " " + word : word;
    if (d->getUTF8Width(cand.c_str()) <= W) { line = cand; }
    else {
      if (line.length()) { if (count >= maxLines) return false; txtBuf[count++] = line; line = ""; }
      while (d->getUTF8Width(word.c_str()) > W) {         // zu langes Wort zeichenweise trennen
        size_t cut = 0, k = 0;
        while (k < word.length()) {
          size_t nk = k + utf8Len(word[k]);
          if (d->getUTF8Width(word.substring(0, nk).c_str()) > W) break;
          k = nk; cut = k;
        }
        if (cut == 0) cut = utf8Len(word[0]);
        if (count >= maxLines) return false;
        txtBuf[count++] = word.substring(0, cut);
        word = word.substring(cut);
      }
      line = word;
    }
    if ((size_t)sp >= s.length()) break;
  }
  if (line.length()) { if (count >= maxLines) return false; txtBuf[count++] = line; }
  return true;
}

void showText(const String& text, uint32_t ms) {
  String s = normalizeText(text);
  s.trim();
  if (s.length() > 200) s = s.substring(0, 200);
  uint8_t cnt = 0;
  bool ok = false;
  for (uint8_t f = 0; f < 5 && !ok; f++) { ok = wrapInto(s, f, cnt); txtFont = f; }
  if (!ok) { startMarquee(s, false); return; }   // zu lang -> als Lauftext
  txtCount = cnt;
  txtUntil = ms ? millis() + ms : 0;
  mode = M_TEXT;
  quietScreen = false;
  touchActivity();
}

void drawText() {
  d->setFont(TXT_FONTS[txtFont]);
  int lh = TXT_LH[txtFont];
  int asc = d->getAscent();
  int total = txtCount * lh;
  int y = (64 - total) / 2 + asc + (lh - asc) / 2 - 1;
  for (uint8_t i = 0; i < txtCount; i++, y += lh) drawCentered(txtBuf[i].c_str(), y);
}

void notify(const String& text) {
  playNotes(SND_NOTIFY, 3);
  vibrate(250);
  startMarquee(text, false);
}

// =====================================================================
//  Schlafmodus, Uhr, Display aus
// =====================================================================
void enterSleep() {
  mode = M_SLEEP;
  emo = E_NEUTRAL; emoUntil = 0;
  clearParticles();
}

void wakeUp() {
  if (mode != M_SLEEP && mode != M_CLOCK && mode != M_OFF) return;
  playNotes(SND_WAKE, 3);
  setEmotion(E_CUTE, 2500);
}

void showClock() {           // Uhr bildschirmfüllend (bleibt bis Doppeltipp)
  mode = M_CLOCK;
  quietScreen = false;
  touchActivity();
}

void enterQuietScreen() {
  if (cfg.quietDisp == 0) return;
  if (cfg.quietDisp == 1) enterSleep();
  else if (cfg.quietDisp == 2) mode = M_CLOCK;
  else mode = M_OFF;
  quietScreen = true;
}

void setQuiet(int8_t v) {   // -1 = Zeitplan, 0 = aus, 1 = an
  quietOverride = v;
  updateQuiet();
  if (quietNow && mode == M_FACE) enterQuietScreen();
}

void drawClockScreen(uint32_t now) {
  int ox = 0, oy = 0;
  if (quietScreen && timeValid()) {        // nachts minimal wandern lassen (gegen Einbrennen)
    time_t t = time(nullptr); struct tm tm; localtime_r(&t, &tm);
    ox = (tm.tm_min % 3) - 1;
  }
  drawPacClock(now, ox, oy);
}

// =====================================================================
//  Gesten
// =====================================================================
void gestureTap() {
  touchActivity();
  switch (mode) {
    case M_SLEEP: case M_OFF: wakeUp(); break;
    case M_CLOCK:   if (quietScreen) wakeUp(); break;          // Uhr bleibt bis Doppeltipp
    case M_INFO:    slideIdx = (slideIdx + 1) % slideCount; beep(2400, 15); break;
    case M_MARQUEE: case M_MSG: case M_TEXT: setEmotion(E_NEUTRAL, 0); break;
    case M_PORTAL:  break;
    default: {
      static const Emotion R[] = {E_CUTE, E_CUTE, E_MINILOVE, E_KISS, E_LEFT, E_RIGHT, E_LAUGH, E_SURPRISED};
      Emotion e = R[random(8)];
      setEmotion(e, 2500);
      playNotes(SND_HAPPY, 2);
      vibrate(40);
    }
  }
}

void gestureDouble() {
  touchActivity();
  switch (mode) {
    case M_SLEEP: case M_CLOCK: case M_OFF: wakeUp(); break;
    case M_INFO:  setEmotion(E_NEUTRAL, 0); beep(1800, 25); break;
    case M_PORTAL: break;
    default:
      if (cfg.lockInfo) { setEmotion(E_CONFUSED, 1500); break; }
      enterInfo(); beep(2200, 25);
  }
}

void gestureTriple() {      // Ruhemodus umschalten
  touchActivity();
  if (mode == M_PORTAL) return;
  bool on = !quietNow;
  quietOverride = on ? 1 : 0;
  updateQuiet();
  showMsg(on ? "Ruhemodus an" : "Ruhemodus aus", on ? "leise & gedimmt" : "", "", 1800);
  if (!on) playNotes(SND_WAKE, 3);
}

void gestureLong() {
  touchActivity();
  switch (mode) {
    case M_SLEEP: case M_OFF: wakeUp(); break;
    case M_CLOCK: if (quietScreen) wakeUp(); else vibrate(40); break;   // Uhr bleibt bis Doppeltipp
    case M_INFO:  vibrate(40); break;                                   // Info bleibt bis Doppeltipp
    case M_PORTAL: break;
    default:
      setEmotion(E_LOVE, 4000);
      playNotes(SND_LOVE, 3);
      vibrate(300);
  }
}

void startPortal();
void stopPortal();

// ---- Schütteln erkennen ----
// Ein Neigungs-/Vibrationsschalter wechselt beim Schütteln viele Male pro Sekunde.
// Einmaliges Kippen = 1 Wechsel -> wird ignoriert.
void onShake() {
  if (mode == M_PORTAL) return;
  uint32_t now = millis();
  shakeCombo = (since(lastShake) < 5000) ? shakeCombo + 1 : 0;
  lastShake = now;
  Mode ret = (mode == M_INFO || (mode == M_CLOCK && !quietScreen) || emoReturn != M_FACE)
             ? (emoReturn != M_FACE ? emoReturn : mode) : M_FACE;
  setEmotion(shakeCombo >= 2 ? E_FIRE : E_ANGRY, 3500);
  emoReturn = ret;
  playNotes(SND_ANGRY, 3);
  vibrate(shakeCombo >= 2 ? 450 : 250);
  Serial.printf("[Schütteln] %s\n", shakeCombo >= 2 ? "Feuer" : "wütend");
}

// Langsames Hin- und Herneigen -> "Uh"-Gesicht (lange weiter schaukeln -> schwindelig)
void onRock() {
  if (mode == M_PORTAL) return;
  rockCombo = (since(lastRock) < 6000) ? rockCombo + 1 : 0;
  lastRock = millis();
  if (mode == M_FACE && (emo == E_UH || emo == E_DIZZY) && emoUntil) {   // schaukelt weiter: verlängern
    if (rockCombo >= 4 && emo != E_DIZZY) {
      Mode ret = emoReturn;
      setEmotion(E_DIZZY, 4000);
      emoReturn = ret;
    } else emoUntil = millis() + (emo == E_DIZZY ? 4000 : 3000);
    return;
  }
  Mode ret = (mode == M_INFO || (mode == M_CLOCK && !quietScreen)) ? mode : M_FACE;
  setEmotion(E_UH, 3000);
  emoReturn = ret;
  playNotes(SND_UH, 3);
  Serial.println("[Schaukeln] uh");
}

// Kontaktwechsel werden per Interrupt erfasst – so geht auch während das Display
// gerade ein Bild sendet (~25 ms) kein Wechsel verloren.
#define TILT_RING 32
volatile uint32_t tiltIsrT[TILT_RING];
volatile uint8_t  tiltIsrHead = 0;
volatile uint32_t tiltIsrLast = 0, tiltEdgeTotal = 0;
uint8_t  tiltTail = 0;
uint32_t vibeQuietUntil = 0;       // Sensor ignorieren, solange der eigene Motor vibriert

void IRAM_ATTR tiltIsr() {
  uint32_t t = millis();
  if (t - tiltIsrLast < 4) return;                     // Kontaktprellen < 4 ms zusammenfassen
  tiltIsrLast = t;
  tiltIsrT[tiltIsrHead % TILT_RING] = t;
  tiltIsrHead = tiltIsrHead + 1;
  tiltEdgeTotal = tiltEdgeTotal + 1;
}

void processTiltEdge(uint32_t now) {
  if ((int32_t)(now - vibeQuietUntil) < 0) return;     // eigene Vibration -> ignorieren
  tiltPos = (tiltPos + 1) % 16;
  tiltEdges[tiltPos] = now;
  static const uint8_t NEED[5] = {12, 9, 7, 5, 4};     // Wechsel pro Sekunde je Empfindlichkeit
  uint8_t need = NEED[constrain(cfg.shakeSens, 1, 5) - 1], cnt = 0;
  for (int i = 0; i < 16; i++) if (tiltEdges[i] && now - tiltEdges[i] <= SHAKE_WINDOW_MS) cnt++;
  if (cnt >= need && since(lastShake) > 1800) {
    for (int i = 0; i < 16; i++) tiltEdges[i] = 0;
    for (int i = 0; i < 8; i++) rockEvents[i] = 0;
    onShake();
    return;
  }
  // langsames Schaukeln: Wechsel zu Ereignissen zusammenfassen (Prellen/Nachrollen der Kugel)
  if (rockEvents[rockPos] && now - rockEvents[rockPos] < 150) return;
  rockPos = (rockPos + 1) % 8;
  rockEvents[rockPos] = now;
  uint8_t fast = 0, slow = 0;
  for (int i = 0; i < 16; i++) if (tiltEdges[i] && now - tiltEdges[i] <= 600) fast++;
  for (int i = 0; i < 8; i++) if (rockEvents[i] && now - rockEvents[i] <= 4000) slow++;
  if (slow >= 4 && fast <= 3 && since(lastShake) > 2000 && since(lastRock) > 1200) onRock();
}

// ---- Automatischer Emotions-Wechsel ----
bool emoSelected(int e) { return (cfg.emoMask >> e) & 1; }

int nextCycleEmotion() {
  int cnt = 0;
  for (int e = 0; e < E_COUNT; e++) if (emoSelected(e)) cnt++;
  if (!cnt) return -1;
  if (cfg.cycleOrder == 1 && cnt > 1) {                  // zufällig, aber nicht zweimal dieselbe
    int e;
    do { e = random(E_COUNT); } while (!emoSelected(e) || e == cycleIdx);
    return e;
  }
  for (int k = 1; k <= E_COUNT; k++) {                   // der Reihe nach
    int e = (cycleIdx + k + E_COUNT) % E_COUNT;
    if (emoSelected(e)) return e;
  }
  return -1;
}

void emoCycleLoop() {
  if (!cfg.cycleMode || quietNow || mode != M_FACE) return;
  uint32_t per = cfg.cycleSec * 1000UL;
  if (cfg.cycleMode == 1) {                              // Durchlauf: eine nach der anderen
    if (!cycleSet && emo != E_NEUTRAL) return;           // manuell gewählte Emotion erst ablaufen lassen
    if (cycleSet && since(cycleLast) < per) return;
    if (since(lastActivity) < 2500) return;              // gerade angetippt -> Reaktion ausspielen lassen
  } else {                                               // im Leerlauf: ab und zu eine Emotion
    if (emo != E_NEUTRAL || emoUntil) return;
    if (since(lastActivity) < per || (cycleLast && since(cycleLast) < per + 4000)) return;
  }
  int e = nextCycleEmotion();
  if (e < 0) return;
  cycleIdx = e;
  cycleLast = millis();
  setEmotion((Emotion)e, cfg.cycleMode == 1 ? per + 400 : 4000, false);
  cycleSet = true;
}

void pollTilt() {
  if (PIN_TILT < 0) return;
  if (mode == M_UPDATE) { tiltTail = tiltIsrHead; return; }
  tiltState = digitalRead(PIN_TILT) == HIGH;
  while (tiltTail != tiltIsrHead) {                     // neue Wechsel aus dem Interrupt abarbeiten
    if ((uint8_t)(tiltIsrHead - tiltTail) > TILT_RING) tiltTail = tiltIsrHead - TILT_RING;   // Überlauf
    uint32_t t = tiltIsrT[tiltTail % TILT_RING];
    tiltTail++;
    if (cfg.shakeOn) processTiltEdge(t);
  }
}

// Wechsel in der letzten Sekunde (für den Live-Sensor-Test)
uint8_t tiltEdgesLastSecond() {
  uint32_t now = millis(); uint8_t n = 0;
  for (int i = 0; i < TILT_RING; i++) { uint32_t t = tiltIsrT[i]; if (t && now - t <= 1000) n++; }
  return n;
}

void pollTouch() {
  if (PIN_TOUCH < 0 || mode == M_UPDATE) return;
  uint32_t now = millis();
  bool raw = digitalRead(PIN_TOUCH) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW);
  if (raw != tState && now - tChange > 20) {
    tState = raw; tChange = now;
    if (raw) {
      tPressAt = now; longFired = false; setupFired = false;
    } else {
      if (!longFired && now - tPressAt < LONG_PRESS_MS) { tapCount++; tReleaseAt = now; }
    }
  }
  if (tState && !longFired && now - tPressAt >= LONG_PRESS_MS) {
    longFired = true; tapCount = 0;
    gestureLong();
  }
  if (tState && !setupFired && now - tPressAt >= SETUP_PRESS_MS) {
    setupFired = true;
    if (portalActive) stopPortal(); else startPortal();
  }
  // Auswertung erst, wenn nach dem letzten Loslassen keine weitere Berührung kam
  if (!tState && tapCount > 0 && now - tReleaseAt > TAP_DOUBLE_GAP) {
    uint8_t n = tapCount;
    tapCount = 0;
    if (n >= 3) gestureTriple();
    else if (n == 2) gestureDouble();
    else gestureTap();
  }
}

// =====================================================================
//  WLAN, Portal, mDNS
// =====================================================================
void sendJson(const String& s, int code = 200);

// =====================================================================
//  Firmware-Update per WLAN (Web-Upload + Arduino-IDE-OTA)
//  Voraussetzung: Partitionsschema mit zwei App-Bereichen,
//  z. B. "Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)".
// =====================================================================
bool     otaStarted = false;
uint8_t  updPct = 0;
bool     updOk = false;
String   updErr;
uint32_t updSize = 0, updDone = 0;

// Achtung: Bei nur EINEM App-Bereich (z. B. "Huge APP") liefert esp_ota_get_next_update_partition()
// den gerade laufenden Bereich zurück – das ist KEIN möglicher Update-Bereich.
bool otaPossible() {
  const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
  const esp_partition_t* run  = esp_ota_get_running_partition();
  return next && run && next->address != run->address;
}

const char* runningPartition() {
  const esp_partition_t* p = esp_ota_get_running_partition();
  return p ? p->label : "?";
}

void drawUpdate() {
  d->setFont(u8g2_font_helvB10_tf);
  drawCentered("Firmware-Update", 16);
  if (updErr.length()) {
    d->setFont(u8g2_font_helvR08_tf);
    drawCentered("Fehler:", 34);
    drawCentered(updErr.c_str(), 48);
    return;
  }
  d->drawRFrame(8, 26, 112, 12, 3);
  d->drawRBox(10, 28, max(1, 108 * updPct / 100), 8, 2);
  char b[40];
  d->setFont(u8g2_font_helvR08_tf);
  if (updOk) drawCentered("Fertig - Neustart...", 54);
  else { snprintf(b, sizeof(b), "%u %%  -  nicht ausschalten!", updPct); drawCentered(b, 54); }
}

void updateScreenNow() {           // während des Updates direkt zeichnen (loop läuft dann kaum)
  setPanelLevel(max<int>(cfg.bright, 50));
  d->setPowerSave(0);
  d->clearBuffer(); drawUpdate(); d->sendBuffer();
}

bool updPwOk() { return !cfg.updPw.length() || server.arg("pw") == cfg.updPw; }

void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    updErr = ""; updOk = false; updPct = 0; updDone = 0;
    updSize = server.hasArg("size") ? server.arg("size").toInt() : 0;
    mode = M_UPDATE; quietScreen = false;
    if (!updPwOk()) { updErr = "Falsches Passwort"; return; }
    if (!otaPossible()) { updErr = "Kein Update-Bereich: einmal per USB mit 'Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)' flashen"; return; }
    Serial.printf("[Update] Start: %s (%lu Bytes)\n", up.filename.c_str(), (unsigned long)updSize);
    if (!Update.begin(updSize ? updSize : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      updErr = Update.errorString();
      if (Update.getError() == UPDATE_ERROR_NO_PARTITION) updErr = "Kein Update-Bereich (Partitionsschema)";
      else if (Update.getError() == UPDATE_ERROR_SIZE) updErr = "Datei zu groß für den Update-Bereich";
    }
    updateScreenNow();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (updErr.length()) return;
    if (updDone == 0) {                                   // Erste Bytes prüfen: ESP32-C3-App-Image?
      const uint8_t* b = up.buf;
      bool magic = up.currentSize > 40 && b[0] == 0xE9;
      bool chip  = magic && b[12] == 0x05 && b[13] == 0x00;            // Chip-ID 5 = ESP32-C3
      bool app   = magic && b[32] == 0x32 && b[33] == 0x54 && b[34] == 0xCD && b[35] == 0xAB;  // App-Beschreibung
      if (!magic || !app) { updErr = "keine App-Firmware (.bin)"; Update.abort(); updateScreenNow(); return; }
      if (!chip) { updErr = "nicht fuer ESP32-C3"; Update.abort(); updateScreenNow(); return; }
    }
    if (Update.write(up.buf, up.currentSize) != up.currentSize) { updErr = Update.errorString(); updateScreenNow(); return; }
    updDone += up.currentSize;
    uint8_t pct = updSize ? min<uint32_t>(99, updDone * 100ULL / updSize) : 50;
    if (pct != updPct) { updPct = pct; updateScreenNow(); }
  } else if (up.status == UPLOAD_FILE_END) {
    if (!updErr.length()) {
      if (Update.end(true)) { updOk = true; updPct = 100; Serial.printf("[Update] OK, %lu Bytes\n", (unsigned long)updDone); }
      else updErr = Update.errorString();
    }
    updateScreenNow();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    updErr = "abgebrochen";
    updateScreenNow();
  }
}

void handleUpdateDone() {
  if (updOk) {
    sendJson("{\"ok\":true}");
    restartAt = millis() + 1500;
  } else {
    sendJson("{\"ok\":false,\"err\":\"" + jsonEsc(updErr.length() ? updErr : String("unbekannter Fehler")) + "\"}", 400);
    showMsg("Update fehlgeschlagen", updErr, "", 5000);
  }
}

void startArduinoOta() {
  if (otaStarted || !otaPossible()) return;
  ArduinoOTA.setHostname(cfg.host.c_str());
  ArduinoOTA.setMdnsEnabled(false);                       // mDNS machen wir selbst (s. startMdns)
  if (cfg.updPw.length()) ArduinoOTA.setPassword(cfg.updPw.c_str());
  ArduinoOTA.onStart([]() { mode = M_UPDATE; updErr = ""; updOk = false; updPct = 0; updateScreenNow(); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    uint8_t pct = t ? p * 100ULL / t : 0;
    if (pct != updPct) { updPct = pct; updateScreenNow(); }
  });
  ArduinoOTA.onEnd([]() { updOk = true; updPct = 100; updateScreenNow(); });
  ArduinoOTA.onError([](ota_error_t e) { updErr = "OTA-Fehler " + String((int)e); updateScreenNow(); restartAt = millis() + 4000; });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.printf("[OTA] bereit: %s.local:3232\n", cfg.host.c_str());
}

void startMdns() {
  MDNS.end();
  if (MDNS.begin(cfg.host.c_str())) {
    MDNS.addService("http", "tcp", 80);
    if (otaPossible()) MDNS.enableArduino(3232, cfg.updPw.length() > 0);   // Netzwerk-Port in der Arduino IDE
    mdnsStarted = true;
    Serial.printf("[mDNS] http://%s.local\n", cfg.host.c_str());
  }
  startArduinoOta();
}

void startPortal() {
  if (portalActive) return;
  Serial.println("[WLAN] Starte Setup-Portal");
  WiFi.mode(WIFI_AP_STA);
  if (strlen(AP_PASS) >= 8) WiFi.softAP(AP_SSID, AP_PASS); else WiFi.softAP(AP_SSID);
#if WIFI_TXPOWER_FIX
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
  delay(100);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  portalActive = true;
  portalSince = millis();
  mode = M_PORTAL;
  quietScreen = false;
  playNotes(SND_NOTIFY, 3);
  vibrate(150);
}

void stopPortal() {
  if (!portalActive) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  portalActive = false;
  if (cfg.ssid.length()) WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  setEmotion(E_NEUTRAL, 0);
}

void drawPortal() {
  d->setFont(u8g2_font_helvB08_tf);
  drawCentered("WLAN-Setup", 9);
  d->drawHLine(0, 12, 128);
  d->setFont(u8g2_font_helvR08_tf);
  d->drawUTF8(0, 25, "1. Verbinden mit WLAN:");
  d->setFont(u8g2_font_helvB08_tf);
  drawCentered(AP_SSID, 37);
  d->setFont(u8g2_font_helvR08_tf);
  d->drawUTF8(0, 50, "2. Im Browser öffnen:");
  d->setFont(u8g2_font_helvB08_tf);
  drawCentered(WiFi.softAPIP().toString().c_str(), 62);
}

void wifiPrepare();
void wifiBegin();

bool connectWifi() {
  if (!cfg.ssid.length()) return false;
  WiFi.mode(WIFI_STA);
  wifiPrepare();
  wifiBegin();
  uint32_t t0 = millis();
  uint8_t dots = 0;
  while (!wifiOk() && millis() - t0 < WIFI_CONNECT_TIMEOUT) {
    d->clearBuffer();
    d->setFont(u8g2_font_helvB10_tf);
    drawCentered("Verbinde WLAN", 24);
    d->setFont(u8g2_font_helvR08_tf);
    drawCentered(cfg.ssid.c_str(), 40);
    for (int i = 0; i < 3; i++) {
      if (i == dots % 3) d->drawDisc(56 + i * 8, 54, 2); else d->drawCircle(56 + i * 8, 54, 2);
    }
    d->sendBuffer();
    dots++;
    delay(250);
  }
  return wifiOk();
}

// =====================================================================
//  WLAN-Wächter
//  Die eingebaute Auto-Neuverbindung des ESP32 bleibt manchmal hängen (z. B. nach
//  Router-Neustart, DSL-Zwangstrennung, Kanalwechsel) oder er hält sich für verbunden,
//  obwohl nichts mehr geht. Darum in Stufen:
//   1. 10 s nach dem Abbruch: selbst neu verbinden (alle 15–60 s wiederholen)
//   2. jeder 4. Versuch: WLAN-Treiber komplett aus- und wieder einschalten
//   3. "verbunden", aber DNS geht 3x nicht: Verbindung bewusst neu aufbauen
//   4. nach WIFI_RESTART_AFTER_MS ohne Verbindung: Neustart des ESP32
// =====================================================================
#ifndef WIFI_RESTART_AFTER_MS
#define WIFI_RESTART_AFTER_MS (20UL * 60UL * 1000UL)
#endif
uint32_t wifiLostSince = 0, wifiLastTry = 0, wifiReconnects = 0;
uint8_t  wifiTries = 0;
volatile bool    wifiForce = false;       // vom DNS-Check gesetzt
volatile uint8_t dnsFails = 0;
volatile int     wifiLastReason = 0;
bool     wifiHardReset = false;

void wifiPrepare() {
  WiFi.setHostname(cfg.host.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);                   // Modem-Schlaf aus: stabiler und schneller erreichbar
}

void wifiBegin() {
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
#if WIFI_TXPOWER_FIX
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
}

bool wifiReallyOk() {
  return wifiOk() && WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

void wifiWatchdog() {
  if (portalActive || !cfg.ssid.length() || mode == M_UPDATE) return;
  uint32_t now = millis();
  if (wifiForce) {                        // verbunden, aber tot -> neu aufbauen
    wifiForce = false;
    Serial.println("[WLAN] verbunden, aber kein Netz (DNS) - baue neu auf");
    WiFi.disconnect(false);
    wifiLostSince = now - 10001;
    wifiLastTry = 0;
  }
  if (wifiReallyOk() && wifiLostSince == 0) { wifiTries = 0; return; }
  if (wifiReallyOk()) {                   // gerade wieder verbunden
    Serial.printf("[WLAN] wieder verbunden nach %lu s\n", (unsigned long)((now - wifiLostSince) / 1000));
    wifiLostSince = 0; wifiTries = 0;
    return;
  }
  if (!wifiLostSince) { wifiLostSince = now; Serial.printf("[WLAN] Verbindung verloren (Grund %d)\n", wifiLastReason); return; }
  uint32_t lost = now - wifiLostSince;
  if (lost > WIFI_RESTART_AFTER_MS) {
    Serial.println("[WLAN] seit langem keine Verbindung - Neustart");
    delay(100);
    ESP.restart();
  }
  if (lost < 10000) return;               // erst der eingebauten Neuverbindung eine Chance geben
  uint32_t interval = min<uint32_t>(60000, 15000 + wifiTries * 5000);
  if (wifiLastTry && now - wifiLastTry < interval) return;
  wifiLastTry = now;
  wifiTries++;
  wifiReconnects++;
  if (wifiTries % 4 == 0) {               // Treiber hängt? komplett neu starten
    Serial.println("[WLAN] Treiber-Reset");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(300);
    WiFi.mode(WIFI_STA);
    wifiPrepare();
    wifiHardReset = true;
  } else {
    Serial.printf("[WLAN] Neuverbindung, Versuch %u\n", wifiTries);
    WiFi.disconnect(false);
    delay(50);
  }
  wifiBegin();
}

// ---- nach erfolgreicher Neuverbindung ----
void wifiOnReconnected() {
  if (wifiHardReset) {                    // nach Treiber-Reset Dienste sauber neu starten
    wifiHardReset = false;
    server.close();
    server.begin();
    if (otaStarted) { ArduinoOTA.end(); otaStarted = false; }
  }
  startMdns();                            // startet auch Arduino-OTA neu
  wxForce = true;
  dnsFails = 0;
}

// =====================================================================
//  Web-Server / API
// =====================================================================
void sendJson(const String& s, int code) { server.send(code, "application/json", s); }
void sendOk() { sendJson("{\"ok\":true}"); }

bool isPortalRequest() {
  return portalActive && server.client().localIP() == WiFi.softAPIP();
}

void handleRoot() {
  if (isPortalRequest() || !cfg.ssid.length()) server.send_P(200, "text/html", PORTAL_HTML);
  else server.send_P(200, "text/html", DASH_HTML);
}

void handleNotFound() {
  if (portalActive) {   // Captive-Portal: alles auf die Setup-Seite umleiten
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Nicht gefunden");
}

void handleStatus() {
  String j; j.reserve(640);
  j += "{\"name\":\"" DEVICE_NAME "\",\"fw\":\"" FW_VERSION "\"";
  j += ",\"host\":\"" + jsonEsc(cfg.host) + "\"";
  j += ",\"ip\":\"" + (wifiOk() ? WiFi.localIP().toString() : (portalActive ? WiFi.softAPIP().toString() : String("-"))) + "\"";
  j += ",\"ssid\":\"" + (wifiOk() ? jsonEsc(WiFi.SSID()) : String("")) + "\"";
  j += ",\"rssi\":" + String(wifiOk() ? WiFi.RSSI() : 0);
  j += ",\"mode\":\"" + String(MODE_NAMES[mode]) + "\"";
  j += ",\"emotion\":\"" + String(EMO_NAMES[emo]) + "\"";
  j += ",\"portal\":" + String(portalActive ? "true" : "false");
  j += ",\"quiet\":" + String(quietNow ? "true" : "false");
  j += ",\"quiet_mode\":\"" + String(quietOverride < 0 ? "auto" : quietOverride ? "on" : "off") + "\"";
  j += ",\"cycle\":" + String(cfg.cycleMode);
  j += ",\"bright\":" + String(panelLevel);
  j += ",\"oled\":" + String(cfg.oled);
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"heap\":" + String(ESP.getFreeHeap());
  j += ",\"ota\":" + String(otaPossible() ? "true" : "false");
  j += ",\"wifi_reconnects\":" + String(wifiReconnects);
  j += ",\"wifi_reason\":" + String((int)wifiLastReason);
  j += ",\"part\":\"" + String(runningPartition()) + "\"";
  j += ",\"upd_pw\":" + String(cfg.updPw.length() ? "true" : "false");
  j += ",\"city\":\"" + jsonEsc(cfg.city) + "\"";
  if (timeValid()) {
    char b[24]; time_t t = time(nullptr); struct tm tm; localtime_r(&t, &tm);
    strftime(b, sizeof(b), "%d.%m.%Y %H:%M:%S", &tm);
    j += ",\"time\":\"" + String(b) + "\"";
  } else j += ",\"time\":null";
  if (wxValid) {
    j += ",\"wx\":{\"temp\":" + String((float)wxTemp, 1) + ",\"code\":" + String((int)wxCode) +
         ",\"hum\":" + String((int)wxHum) + ",\"wind\":" + String((float)wxWind, 1) +
         ",\"text\":\"" + String(wxText(wxCode)) + "\"}";
  } else j += ",\"wx\":null";
  j += "}";
  sendJson(j);
}

void handleGetSettings() {
  String j; j.reserve(700);
  auto B = [](bool v) { return v ? "true" : "false"; };
  char hm[8];
  j += "{\"ssid\":\"" + jsonEsc(cfg.ssid) + "\"";
  j += ",\"host\":\"" + jsonEsc(cfg.host) + "\"";
  j += ",\"city\":\"" + jsonEsc(cfg.city) + "\"";
  j += ",\"tz\":\"" + jsonEsc(cfg.tz) + "\"";
  j += ",\"oled\":" + String(cfg.oled);
  j += ",\"flip\":" + String(B(cfg.flip));
  j += ",\"bright\":" + String(cfg.bright);
  j += ",\"quiet_on\":" + String(B(cfg.quietOn));
  snprintf(hm, sizeof(hm), "%02u:%02u", cfg.quietFrom / 60, cfg.quietFrom % 60);
  j += ",\"quiet_from\":\"" + String(hm) + "\"";
  snprintf(hm, sizeof(hm), "%02u:%02u", cfg.quietTo / 60, cfg.quietTo % 60);
  j += ",\"quiet_to\":\"" + String(hm) + "\"";
  j += ",\"quiet_bright\":" + String(cfg.quietBright);
  j += ",\"quiet_mute\":" + String(B(cfg.quietMute));
  j += ",\"quiet_novibe\":" + String(B(cfg.quietNoVibe));
  j += ",\"quiet_disp\":" + String(cfg.quietDisp);
  j += ",\"lat\":" + String(cfg.lat, 4);
  j += ",\"lon\":" + String(cfg.lon, 4);
  j += ",\"sl_weather\":" + String(B(cfg.slWeather));
  j += ",\"sl_clock\":" + String(B(cfg.slClock));
  j += ",\"sl_wifi\":" + String(B(cfg.slWifi));
  j += ",\"sl_ip\":" + String(B(cfg.slIp));
  j += ",\"sl_fw\":" + String(B(cfg.slFw));
  j += ",\"lock_info\":" + String(B(cfg.lockInfo));
  j += ",\"sound\":" + String(B(cfg.sound));
  j += ",\"vibe\":" + String(B(cfg.vibe));
  j += ",\"pac_sound\":" + String(B(cfg.pacSound));
  j += ",\"shake_on\":" + String(B(cfg.shakeOn));
  j += ",\"shake_sens\":" + String(cfg.shakeSens);
  j += ",\"cycle_mode\":" + String(cfg.cycleMode);
  j += ",\"cycle_order\":" + String(cfg.cycleOrder);
  j += ",\"cycle_sec\":" + String(cfg.cycleSec);
  j += ",\"emo_mask\":" + String((unsigned long)cfg.emoMask);
  j += ",\"mq_speed\":" + String(cfg.mqSpeed);
  j += ",\"sleep_min\":" + String(cfg.sleepMin) + "}";
  sendJson(j);
}

uint16_t parseHM(const String& s, uint16_t def) {   // "22:30" -> Minuten
  int c = s.indexOf(':');
  if (c < 1) return def;
  int h = s.substring(0, c).toInt(), m = s.substring(c + 1).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return def;
  return h * 60 + m;
}

bool argBool(const char* k, bool& v) {
  if (!server.hasArg(k)) return false;
  String a = server.arg(k);
  v = (a == "1" || a == "true" || a == "on");
  return true;
}

void handleSaveSettings() {
  bool restart = false, mdnsChange = false;
  if (server.hasArg("host")) {
    String h = sanitizeHost(server.arg("host"));
    if (h != cfg.host) { cfg.host = h; mdnsChange = true; }
  }
  if (server.hasArg("oled")) {
    uint8_t o = server.arg("oled").toInt() == 1 ? 1 : 0;
    if (o != cfg.oled) { cfg.oled = o; restart = true; }
  }
  if (argBool("flip", cfg.flip)) d->setFlipMode(cfg.flip ? 1 : 0);
  if (server.hasArg("bright")) cfg.bright = constrain(server.arg("bright").toInt(), 1, 100);
  argBool("quiet_on", cfg.quietOn);
  if (server.hasArg("quiet_from")) cfg.quietFrom = parseHM(server.arg("quiet_from"), cfg.quietFrom);
  if (server.hasArg("quiet_to"))   cfg.quietTo   = parseHM(server.arg("quiet_to"), cfg.quietTo);
  if (server.hasArg("quiet_bright")) cfg.quietBright = constrain(server.arg("quiet_bright").toInt(), 1, 100);
  argBool("quiet_mute", cfg.quietMute);
  argBool("quiet_novibe", cfg.quietNoVibe);
  if (server.hasArg("quiet_disp")) cfg.quietDisp = constrain(server.arg("quiet_disp").toInt(), 0, 3);
  if (server.hasArg("city")) { cfg.city = server.arg("city"); cfg.city.trim(); if (cfg.city.length() > 32) cfg.city = cfg.city.substring(0, 32); }
  bool locChange = false;
  if (server.hasArg("lat")) { float v = server.arg("lat").toFloat(); if (v >= -90 && v <= 90 && v != cfg.lat) { cfg.lat = v; locChange = true; } }
  if (server.hasArg("lon")) { float v = server.arg("lon").toFloat(); if (v >= -180 && v <= 180 && v != cfg.lon) { cfg.lon = v; locChange = true; } }
  if (server.hasArg("tz") && server.arg("tz").length() && server.arg("tz") != "custom") {
    String tz = server.arg("tz"); tz.trim();
    if (tz != cfg.tz && tz.length() < 64) { cfg.tz = tz; setenv("TZ", cfg.tz.c_str(), 1); tzset(); pcLastDraw = 0; }
  }
  argBool("sl_weather", cfg.slWeather);
  argBool("sl_clock", cfg.slClock);
  argBool("sl_wifi", cfg.slWifi);
  argBool("sl_ip", cfg.slIp);
  argBool("sl_fw", cfg.slFw);
  argBool("lock_info", cfg.lockInfo);
  argBool("sound", cfg.sound);
  argBool("vibe", cfg.vibe);
  argBool("pac_sound", cfg.pacSound);
  argBool("shake_on", cfg.shakeOn);
  if (server.hasArg("shake_sens")) cfg.shakeSens = constrain(server.arg("shake_sens").toInt(), 1, 5);
  if (server.hasArg("cycle_mode"))  { cfg.cycleMode = constrain(server.arg("cycle_mode").toInt(), 0, 2); cycleLast = 0; }
  if (server.hasArg("cycle_order")) cfg.cycleOrder = server.arg("cycle_order").toInt() ? 1 : 0;
  if (server.hasArg("cycle_sec"))   cfg.cycleSec = constrain(server.arg("cycle_sec").toInt(), 2, 60);
  if (server.hasArg("emo_mask"))    cfg.emoMask = strtoul(server.arg("emo_mask").c_str(), nullptr, 10);
  if (server.hasArg("mq_speed")) cfg.mqSpeed = constrain(server.arg("mq_speed").toInt(), 1, 10);
  if (server.hasArg("sleep_min")) cfg.sleepMin = constrain(server.arg("sleep_min").toInt(), 0, 240);
  if (server.hasArg("upd_pw")) {                          // leer = kein Passwort
    String pw = server.arg("upd_pw");
    if (pw.length() > 32) pw = pw.substring(0, 32);
    if (pw != cfg.updPw) { cfg.updPw = pw; if (otaStarted) ArduinoOTA.setPassword(cfg.updPw.length() ? cfg.updPw.c_str() : nullptr); if (wifiOk()) startMdns(); }
  }
  saveSettings();
  if (locChange) { wxValid = false; wxForce = true; }
  if (mode == M_INFO) { buildSlides(); slideIdx = 0; }
  if (mdnsChange && wifiOk() && !restart) startMdns();
  sendJson(String("{\"ok\":true,\"restart\":") + (restart ? "true" : "false") + ",\"host\":\"" + jsonEsc(cfg.host) + "\"}");
  if (restart) restartAt = millis() + 800;
}

void handleScan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true); sendJson("{\"status\":\"scanning\"}"); return; }
  if (n == WIFI_SCAN_RUNNING) { sendJson("{\"status\":\"scanning\"}"); return; }
  String j = "{\"status\":\"done\",\"networks\":[";
  bool first = true;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (int k = 0; k < i; k++) if (WiFi.SSID(k) == s) { dup = true; break; }
    if (dup) continue;
    if (!first) j += ",";
    first = false;
    j += "{\"ssid\":\"" + jsonEsc(s) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
         ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  j += "]}";
  WiFi.scanDelete();
  sendJson(j);
}

void handleWifiSave() {
  String ssid = server.arg("ssid"); ssid.trim();
  if (!ssid.length() || ssid.length() > 32) { sendJson("{\"ok\":false,\"err\":\"SSID fehlt\"}", 400); return; }
  String pass = server.arg("pass");
  if (pass.length() && pass.length() < 8) { sendJson("{\"ok\":false,\"err\":\"Passwort muss mind. 8 Zeichen haben\"}", 400); return; }
  cfg.ssid = ssid;
  cfg.pass = pass;
  if (server.hasArg("host")) cfg.host = sanitizeHost(server.arg("host"));
  if (server.hasArg("oled")) cfg.oled = server.arg("oled").toInt() == 1 ? 1 : 0;
  saveSettings();
  sendJson("{\"ok\":true,\"host\":\"" + jsonEsc(cfg.host) + "\"}");
  showMsg("Gespeichert", "Neustart...", cfg.ssid, 5000);
  restartAt = millis() + 1500;
}

uint32_t argMs(uint32_t def) { return server.hasArg("ms") ? (uint32_t)server.arg("ms").toInt() : def; }

void setupServer() {
  server.enableCORS(true);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/dashboard", HTTP_GET, []() { server.send_P(200, "text/html", DASH_HTML); });
  server.on("/setup", HTTP_GET, []() { server.send_P(200, "text/html", PORTAL_HTML); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);

  server.on("/api/emotion", []() {
    int e = parseEmotion(server.arg("name"));
    if (e < 0) { sendJson("{\"ok\":false,\"err\":\"unbekannte Emotion\"}", 400); return; }
    setEmotion((Emotion)e, argMs(10000));
    sendOk();
  });
  server.on("/api/tap",    []() { gestureTap(); sendOk(); });
  server.on("/api/shake",  []() { onShake(); sendOk(); });
  server.on("/api/rock",   []() { onRock(); sendOk(); });
  server.on("/api/cycle",  []() {                   // ?mode=off|run|idle  [&sec=6] [&order=seq|random]
    String m = server.arg("mode");
    if (m == "off" || m == "0") cfg.cycleMode = 0;
    else if (m == "idle" || m == "2") cfg.cycleMode = 2;
    else if (m.length()) cfg.cycleMode = 1;
    if (server.hasArg("sec")) cfg.cycleSec = constrain(server.arg("sec").toInt(), 2, 60);
    if (server.hasArg("order")) cfg.cycleOrder = server.arg("order") == "random" ? 1 : 0;
    cycleLast = 0;
    if (!cfg.cycleMode && mode == M_FACE) setEmotion(E_NEUTRAL, 0);
    saveSettings();
    sendJson("{\"ok\":true,\"cycle\":" + String(cfg.cycleMode) + "}");
  });
  server.on("/api/sensor", HTTP_GET, []() {             // Live-Sensor-Test
    String j = "{\"tilt\":" + String(PIN_TILT >= 0 ? (tiltState ? 1 : 0) : -1) +
               ",\"edges\":" + String((unsigned long)tiltEdgeTotal) +
               ",\"per_sec\":" + String(tiltEdgesLastSecond()) +
               ",\"touch\":" + String(tState ? 1 : 0) +
               ",\"last\":\"" + String(since(lastShake) < 4000 ? "schuetteln" : since(lastRock) < 4000 ? "schaukeln" : "") + "\"}";
    sendJson(j);
  });
  server.on("/api/double", []() { gestureDouble(); sendOk(); });
  server.on("/api/long",   []() { gestureLong(); sendOk(); });
  server.on("/api/info",   []() { enterInfo(); sendOk(); });
  server.on("/api/clock",  []() { showClock(); sendOk(); });
  server.on("/api/clock/demo", []() { showClock(); pcDemo(); sendOk(); });
  server.on("/api/sleep",  []() { enterSleep(); sendOk(); });
  server.on("/api/wake",   []() { wakeUp(); if (mode != M_FACE) setEmotion(E_NEUTRAL, 0); sendOk(); });
  server.on("/api/vibrate", []() { vibrate(constrain((int)argMs(300), 10, 3000)); sendOk(); });
  server.on("/api/beep", []() {
    uint16_t f = server.hasArg("f") ? constrain(server.arg("f").toInt(), 100, 8000) : 2800;
    beep(f, constrain((int)argMs(120), 10, 2000));
    sendOk();
  });
  auto textHandler = []() {
    String t = server.arg("text");
    if (!t.length()) { sendJson("{\"ok\":false,\"err\":\"text fehlt\"}", 400); return; }
    if (server.hasArg("speed")) cfg.mqSpeed = constrain(server.arg("speed").toInt(), 1, 10);
    String m = server.arg("mode");
    if (m == "static") showText(t, argMs(15000));
    else startMarquee(t, m == "loop");
    sendOk();
  };
  server.on("/api/text", textHandler);
  server.on("/api/marquee", textHandler);
  server.on("/api/notify", []() {
    String t = server.arg("text");
    if (!t.length()) t = "Neue Nachricht";
    notify(t);
    sendOk();
  });
  server.on("/api/dim", []() {            // ?level=1..100 [&save=1]
    if (!server.hasArg("level")) { sendJson("{\"ok\":false,\"err\":\"level fehlt\"}", 400); return; }
    int lv = constrain(server.arg("level").toInt(), 1, 100);
    if (quietNow) cfg.quietBright = lv; else cfg.bright = lv;
    if (server.arg("save") == "1") saveSettings();
    sendOk();
  });
  server.on("/api/quiet", []() {          // ?state=on|off|auto|toggle
    String st = server.arg("state");
    if (st == "on") setQuiet(1);
    else if (st == "off") setQuiet(0);
    else if (st == "auto") setQuiet(-1);
    else setQuiet(quietNow ? 0 : 1);
    if (!quietNow && quietScreen) wakeUp();
    sendJson(String("{\"ok\":true,\"quiet\":") + (quietNow ? "true" : "false") + "}");
  });
  server.on("/api/portal",  []() { sendOk(); startPortal(); });
  server.on("/api/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.on("/api/restart", []() { sendOk(); restartAt = millis() + 500; });
  server.on("/api/reset",   []() {
    sendOk();
    factoryReset();
    showMsg("Zurückgesetzt", "Neustart...", "", 3000);
    restartAt = millis() + 800;
  });
  server.onNotFound(handleNotFound);
  server.begin();
}

// =====================================================================
//  USB-Seriell-Befehle
// =====================================================================
void handleSerialCmd(String line) {
  line.trim();
  if (!line.length()) return;
  int sp = line.indexOf(' ');
  String cmd = sp < 0 ? line : line.substring(0, sp);
  String arg = sp < 0 ? "" : line.substring(sp + 1);
  cmd.toLowerCase(); arg.trim();

  if (cmd == "emotion" || cmd == "emo") {
    int s2 = arg.indexOf(' ');
    String name = s2 < 0 ? arg : arg.substring(0, s2);
    uint32_t ms = s2 < 0 ? 10000 : arg.substring(s2 + 1).toInt();
    int e = parseEmotion(name);
    if (e < 0) { Serial.println("ERR unbekannte Emotion"); return; }
    setEmotion((Emotion)e, ms);
  }
  else if (cmd == "text")   startMarquee(arg, false);
  else if (cmd == "loop")   startMarquee(arg, true);
  else if (cmd == "show")   showText(arg, 15000);
  else if (cmd == "speed")  cfg.mqSpeed = constrain((int)arg.toInt(), 1, 10);
  else if (cmd == "notify") notify(arg.length() ? arg : String("Neue Nachricht"));
  else if (cmd == "beep")   beep(arg.length() ? arg.toInt() : 2800, 120);
  else if (cmd == "vibe")   vibrate(arg.length() ? arg.toInt() : 300);
  else if (cmd == "dim")    { int lv = constrain((int)arg.toInt(), 1, 100); if (quietNow) cfg.quietBright = lv; else cfg.bright = lv; }
  else if (cmd == "quiet")  { if (arg == "on") setQuiet(1); else if (arg == "off") setQuiet(0); else if (arg == "auto") setQuiet(-1); else setQuiet(quietNow ? 0 : 1); }
  else if (cmd == "tap")    gestureTap();
  else if (cmd == "shake")  onShake();
  else if (cmd == "rock")   onRock();
  else if (cmd == "cycle")  { cfg.cycleMode = arg == "off" ? 0 : arg == "idle" ? 2 : 1; cycleLast = 0; saveSettings(); }
  else if (cmd == "double") gestureDouble();
  else if (cmd == "long")   gestureLong();
  else if (cmd == "info")   enterInfo();
  else if (cmd == "clock")  { showClock(); if (arg == "demo") pcDemo(); }
  else if (cmd == "sleep")  enterSleep();
  else if (cmd == "wake")   wakeUp();
  else if (cmd == "portal") startPortal();
  else if (cmd == "restart") ESP.restart();
  else if (cmd == "status") {
    Serial.printf("mode=%s emotion=%s ip=%s host=%s.local quiet=%d bright=%d\n", MODE_NAMES[mode], EMO_NAMES[emo],
                  WiFi.localIP().toString().c_str(), cfg.host.c_str(), quietNow, panelLevel);
    return;
  }
  else if (cmd == "help") {
    Serial.println("Befehle: emotion <name> [ms] | text <t> | loop <t> | show <t> | speed <1-10> | notify <t>");
    Serial.println("         beep [f] | vibe [ms] | dim <1-100> | quiet on|off|auto|toggle | clock [demo]");
    Serial.println("         tap | double | long | shake | info | sleep | wake | portal | restart | status");
    Serial.print("Emotionen:");
    for (int i = 0; i < E_COUNT; i++) { Serial.print(' '); Serial.print(EMO_NAMES[i]); }
    Serial.println();
    return;
  }
  else { Serial.println("ERR unbekannter Befehl (help)"); return; }
  Serial.println("OK");
}

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { if (serBuf.length()) handleSerialCmd(serBuf); serBuf = ""; }
    else if (serBuf.length() < 220) serBuf += c;
  }
}

// =====================================================================
//  Rendern
// =====================================================================
void drawMsg() {
  d->setFont(u8g2_font_helvB10_tf);
  drawCentered(msgL1.c_str(), 20);
  d->setFont(u8g2_font_helvR08_tf);
  drawCentered(msgL2.c_str(), 38);
  drawCentered(msgL3.c_str(), 52);
}

void render(uint32_t now) {
  if ((mode == M_OFF) != panelOff) { panelOff = (mode == M_OFF); d->setPowerSave(panelOff ? 1 : 0); }
  if (panelOff) return;
  d->clearBuffer();
  switch (mode) {
    case M_FACE:
    case M_SLEEP:   drawFace(now); break;
    case M_INFO:    drawInfo(now); break;
    case M_MARQUEE: drawMarquee(); break;
    case M_PORTAL:  drawPortal(); break;
    case M_MSG:     drawMsg(); break;
    case M_TEXT:    drawText(); break;
    case M_CLOCK:   drawClockScreen(now); break;
    case M_OFF:     break;
    case M_UPDATE:  drawUpdate(); break;
  }
  // Fortschrittsbalken beim Halten für das Setup-Portal
  if (tState && since(tPressAt) > 1500 && !setupFired) {
    int w = map(min<uint32_t>(since(tPressAt), SETUP_PRESS_MS), 1500, SETUP_PRESS_MS, 0, 124);
    d->setDrawColor(0); d->drawBox(0, 48, 128, 16); d->setDrawColor(1);
    d->setFont(u8g2_font_helvR08_tf);
    drawCentered(portalActive ? "Halten: Setup beenden" : "Halten: WLAN-Setup", 56);
    d->drawFrame(0, 58, 128, 6);
    d->drawBox(2, 60, w, 2);
  }
  applyDimMask();
  d->sendBuffer();
}

void bootAnimation() {
  for (int i = 0; i <= 12; i++) {
    d->clearBuffer();
    float h = 2 + i * 2.8f;
    drawEyeBox(38, 26, 30, h);
    drawEyeBox(90, 26, 30, h);
    d->setFont(u8g2_font_helvB08_tf);
    drawCentered(DEVICE_NAME, 58);
    d->sendBuffer();
    delay(30);
  }
  playNotes(SND_BOOT, 2);
}

// =====================================================================
//  setup / loop
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("\n[PCBuddy] v" FW_VERSION);

  if (PIN_TOUCH >= 0)  pinMode(PIN_TOUCH, TOUCH_ACTIVE_HIGH ? INPUT : INPUT_PULLUP);
  if (PIN_TILT >= 0)   {
    pinMode(PIN_TILT, INPUT_PULLUP);
    tiltState = digitalRead(PIN_TILT) == HIGH;
    attachInterrupt(digitalPinToInterrupt(PIN_TILT), tiltIsr, CHANGE);
  }
  if (PIN_VIBRO >= 0)  { pinMode(PIN_VIBRO, OUTPUT); digitalWrite(PIN_VIBRO, LOW); }
  if (PIN_BUZZER >= 0) { pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW); }

  loadSettings();
  randomSeed(esp_random());
  initDisplay();
  bootAnimation();

  WiFi.persistent(false);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) { wifiLastReason = info.wifi_sta_disconnected.reason; },
               ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);
  setenv("TZ", cfg.tz.c_str(), 1);
  tzset();
  configTzTime(cfg.tz.c_str(), "pool.ntp.org", "time.google.com");

  if (!cfg.ssid.length()) {
    startPortal();
  } else if (connectWifi()) {
    Serial.printf("[WLAN] Verbunden: %s  IP %s\n", cfg.ssid.c_str(), WiFi.localIP().toString().c_str());
    startMdns();
    showMsg("Verbunden!", WiFi.localIP().toString(), cfg.host + ".local", 4000);
  } else {
    Serial.println("[WLAN] Verbindung fehlgeschlagen - Offline-Modus");
    showMsg("Offline-Modus", "WLAN nicht erreichbar", "5 s halten = Setup", 4000);
  }

  setupServer();
  xTaskCreate(weatherTask, "weather", 10240, nullptr, 1, nullptr);
  lastActivity = millis();
  nextBlink = millis() + 2000;
}

void loop() {
  server.handleClient();
  if (otaStarted) ArduinoOTA.handle();
  if (portalActive) dns.processNextRequest();
  pollTouch();
  pollTilt();
  pollSerial();
  soundLoop();
  vibeLoop();
  displayHealth();

  // WICHTIG: Zeit erst NACH den Eingaben lesen – Handler setzen Zeitstempel mit millis(),
  // die sonst "in der Zukunft" lägen und bei uint32-Differenzen überlaufen würden.
  uint32_t now = millis();

  wifiWatchdog();
  static bool wasOk = wifiOk();
  bool ok = wifiOk();
  if (ok && !wasOk) wifiOnReconnected();
  wasOk = ok;

  if (portalActive && cfg.ssid.length() && since(portalSince) > 600000UL) ESP.restart();

  // Ruhemodus nach Zeitplan / manuell
  static uint32_t lastQuietCheck = 0;
  if (since(lastQuietCheck) > 1000) {
    lastQuietCheck = now;
    bool was = quietNow;
    updateQuiet();
    if (quietNow && !was && mode == M_FACE) enterQuietScreen();
    if (!quietNow && was && quietScreen) setEmotion(E_NEUTRAL, 0, false);
  }

  switch (mode) {
    case M_MSG:
      if ((int32_t)(now - msgUntil) > 0) {
        if (msgReturn == M_INFO || msgReturn == M_CLOCK) { mode = msgReturn; msgReturn = M_FACE; }
        else setEmotion(E_NEUTRAL, 0, false);
      }
      break;
    case M_TEXT:    if (txtUntil && (int32_t)(now - txtUntil) > 0) setEmotion(E_NEUTRAL, 0, false); break;
    // Info-Folien und Uhr bleiben offen, bis doppelt getippt wird
    case M_MARQUEE: marqueeLoop(); break;
    case M_FACE:
      emoCycleLoop();
      if (quietNow && cfg.quietDisp > 0 && !emoUntil && since(lastActivity) > 20000) enterQuietScreen();
      else if (!quietNow && cfg.sleepMin && emo == E_NEUTRAL && since(lastActivity) > cfg.sleepMin * 60000UL) enterSleep();
      break;
    default: break;
  }
  if (mode == M_FACE || mode == M_SLEEP) updateFace(now);
  updateBrightness();

  static uint32_t lastFrame = 0;
  if (since(lastFrame) >= 33) { lastFrame = now; render(now); }

  if (restartAt && (int32_t)(now - restartAt) > 0) { delay(100); ESP.restart(); }
  delay(1);
}
