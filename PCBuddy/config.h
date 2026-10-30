#pragma once
// =====================================================================
//  PCBuddy – Hardware-Konfiguration
//  ESP32-C3 SuperMini + OLED 128x64 (0,96" SSD1306 oder 1,3" SH1106)
// =====================================================================

#define FW_VERSION      "1.6.4"
#define DEVICE_NAME     "PCBuddy"

// ---------- I2C OLED ----------
// Standard-I2C des ESP32-C3 SuperMini: SDA = GPIO8, SCL = GPIO9
#define PIN_SDA         8
#define PIN_SCL         9
#define OLED_ADDR       0x3C      // 0x3C (meist) oder 0x3D – wird beim Start automatisch geprüft
#define OLED_I2C_HZ     400000

// ---------- Ein-/Ausgänge (-1 = nicht vorhanden) ----------
#define PIN_TOUCH       7         // TTP223 Touch-Sensor (oder Taster)
#define TOUCH_ACTIVE_HIGH 1       // TTP223 = 1 (HIGH bei Berührung); Taster gegen GND = 0
#define PIN_BUZZER      5         // passiver Piezo-Buzzer
#define PIN_VIBRO       10        // Vibrationsmotor (über Transistor/MOSFET!)
#define PIN_TILT        1         // Neigungs-/Erschütterungssensor (SW-520D, SW-18010P …) gegen GND
#define SHAKE_WINDOW_MS 1000      // Zeitfenster für die Schüttel-Erkennung

// ---------- Gesten-Timing (ms) ----------
#define TAP_DOUBLE_GAP  450       // max. Pause zwischen zwei Taps (Doppel-/Dreifachtipp)
#define LONG_PRESS_MS   650       // langes Drücken
#define SETUP_PRESS_MS  5000      // so lange halten -> WLAN-Setup-Portal

// ---------- WLAN ----------
#define AP_SSID         "PCBuddy-Setup"
#define AP_PASS         ""        // leer = offenes Netz, sonst min. 8 Zeichen
#define WIFI_CONNECT_TIMEOUT 15000
// Viele SuperMini-Boards haben eine schwache Antennen-Anpassung und verbinden
// sich mit voller Sendeleistung nicht. 1 = Sendeleistung auf 8,5 dBm reduzieren.
#define WIFI_TXPOWER_FIX 1

// ---------- Standardwerte ----------
#define DEFAULT_HOST    "pcbuddy"
#define DEFAULT_CITY    "Berlin"
#define DEFAULT_LAT     52.5200f
#define DEFAULT_LON     13.4050f
#define DEFAULT_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"   // Deutschland / Mitteleuropa

#define WEATHER_INTERVAL_MS (15UL * 60UL * 1000UL)
