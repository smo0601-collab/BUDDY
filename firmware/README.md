# Fertige Firmware für das Update per WLAN

| Datei | Version |
|---|---|
| [`PCBuddy_update_v1.6.4.bin`](PCBuddy_update_v1.6.4.bin) | v1.6.4 (aktuell) |

**So installierst du sie:** Datei herunterladen (Datei anklicken → *Download raw file*) → Dashboard
`http://pcbuddy.local` → **System → Firmware-Update per WLAN** → Datei wählen → *Update installieren*.

Voraussetzung: PCBuddy wurde einmal per USB mit **Board „ESP32C3 Dev Module“** und
**Partition Scheme „Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)“** geflasht (siehe [README](../README.md#installation)).

Die Datei enthält nur das Programm (App-Image), nicht den Bootloader. Für eine Neuinstallation per USB den Sketch in der
Arduino IDE hochladen.
