# Fertige Firmware

Am einfachsten lädst du die Dateien aus dem [**neuesten Release**](../../../releases/latest). Hier liegen dieselben Dateien.

| Datei | Wofür | Einstellungen |
|---|---|---|
| [`PCBuddy_full_v1.6.4.bin`](PCBuddy_full_v1.6.4.bin) | **Komplette Firmware** für die Erstinstallation per USB, an Adresse **0x0** flashen | werden zurückgesetzt |
| [`PCBuddy_update_v1.6.4.bin`](PCBuddy_update_v1.6.4.bin) | **Update per WLAN** für ein schon eingerichtetes Gerät | bleiben erhalten |

## Erstinstallation ohne Arduino IDE (Komplett-Firmware)

1. Das [**ESP Web Tool**](https://espressif.github.io/esptool-js/) in **Chrome oder Edge** öffnen und den ESP32-C3 SuperMini per USB-C anschließen.
2. **Connect** → seriellen Port wählen. Klappt die Verbindung nicht: **BOOT** gedrückt halten, kurz **RESET** drücken, BOOT loslassen.
3. **Erase Flash** (empfohlen), dann bei *Flash Address* **`0x0`** eintragen, `PCBuddy_full_v1.6.4.bin` wählen → **Program**.
4. Nach dem Neustart mit dem WLAN **`PCBuddy-Setup`** verbinden und einrichten (siehe [README](../README.md#ersteinrichtung)).

Per Kommandozeile geht es auch:

```bash
esptool.py --chip esp32c3 erase_flash
esptool.py --chip esp32c3 write_flash 0x0 PCBuddy_full_v1.6.4.bin
```

Die Komplett-Datei enthält Bootloader, Partitionstabelle (**Minimal SPIFFS, 2 × 1,9 MB für Updates per WLAN**) und
Programm. Danach funktionieren alle weiteren Updates per WLAN.

## Update per WLAN (Update-Datei)

Dashboard `http://pcbuddy.local` → **System → Firmware-Update per WLAN** → `PCBuddy_update_v1.6.4.bin` wählen →
*Update installieren*. Voraussetzung: PCBuddy läuft schon mit dem Partitionsschema „Minimal SPIFFS“. Das ist nach
der Komplett-Firmware oder nach dem Flashen per Arduino IDE mit diesem Schema der Fall.

> Die Komplett-Datei (`…_full_…`) lässt sich **nicht** per WLAN einspielen. PCBuddy lehnt sie dort ab, damit nichts
> kaputtgehen kann.
