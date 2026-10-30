# Changelog

Alle Versionen von PCBuddy. Die neueste steht oben.

## v1.6.4

- WLAN-Wächter: verbindet sich nach Router-Neustart oder Abbruch selbst wieder (Neuverbindung, Treiber-Reset, DNS-Prüfung, Neustart nach 20 min); WLAN-Energiesparen aus; Zähler/Grund im Dashboard.

## v1.6.3

- Beschriftung des Partitionsschemas korrigiert: In aktuellen Arduino-ESP32-Versionen heißt es „Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)“ (ältere Versionen schrieben fälschlich 190KB, gleiche Aufteilung).

## v1.6.2

- Update-Prüfung erkennt „Huge APP“ jetzt richtig (vorher fälschlich „Update möglich“ → „Partition Could Not be Found“), klare Hinweise im Dashboard; README: Board muss „ESP32C3 Dev Module“ sein.

## v1.6.1

- 1,3"-SH1106: Display geht beim Piepen/Vibrieren nicht mehr dauerhaft aus (automatische Neu-Initialisierung nach Ton/Vibration, alle 15 s und bei I2C-Ausfall); Dimmen am SH1106 ohne Register-Eingriffe (Kontrast + Pixel-Dimmung). 0,96"-SSD1306 unverändert.

## v1.6

- Firmware-Update per WLAN (Dashboard-Upload mit Prüfung + Arduino-IDE-OTA), optionales Update-Passwort. Partitionsschema jetzt „Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)“ (einmal per USB flashen).

## v1.5.3

- Ruhemodus und Dimmen wieder exakt wie in v1.4.3 (hat sich in der Praxis besser bewährt); die Änderungen aus v1.5.1/v1.5.2 am Ruhemodus sind zurückgenommen. Emotions-Wechsel aus v1.5 bleibt.

## v1.5.2

- Ruhemodus beim 1,3"-SH1106: Dimmen jetzt per Software (Pixelmuster) statt über Controller-Register. Der Ruhemodus wirkt nach dem 3× Tippen sofort und nicht erst nach 20 s.

## v1.5.1

- Fix: Display blieb nach dem Ruhemodus dunkel (SH1106-Register wurden falsch zurückgesetzt). Jetzt wird beim Verlassen der Tiefdimmung die Original-Initialisierung des Controllers neu gesendet. Im Ruhemodus wird das Display beim Antippen 20 s lang gut sichtbar (40 %), und die Tiefdimmung ist weniger extrem.

## v1.5

- Automatischer Emotions-Wechsel (Durchlauf oder im Leerlauf), Emotionen einzeln auswählbar

## v1.4.3

- Waka-Waka nur noch zur vollen Stunde

## v1.4.2

- Neigungssensor per Interrupt (kein Wechsel geht mehr verloren, während das Display ein Bild sendet), eigene Vibration löst kein Schütteln mehr aus, Live-Sensor-Test im Dashboard (`/api/sensor`)

## v1.4.1

- Langsames Hin- und Herneigen gibt ein „Uh“-Gesicht (lange schaukeln = schwindelig), neue Emotion „uh“

## v1.4

- Touch-Sensor auf GPIO 7, Neigungssensor auf GPIO 1: Schütteln macht wütend (mehrfach = Feuer), Empfindlichkeit einstellbar

## v1.3.1

- Pac-Man berührt die Uhrzeit-Pellets beim Herumlaufen nicht mehr, frisst nur die umschlagende Ziffer

## v1.3

- Pac-Man läuft frei über den ganzen Bildschirm und frisst nur kurz vor dem Wechsel; Info-Folien und Uhr bleiben bis Doppeltipp

## v1.2.1

- Pac-Man frisst die Ziffer kurz vor dem Wechsel, die neue erscheint pünktlich zur vollen Minute

## v1.2

- Pac-Man-Uhr mit Fress-Animation, Sekunden-Bahn und optionalem Waka-Waka-Ton
- 6 neue Emotionen: pochendes Herz, lachen, begeistert, überrascht, verwirrt, schwindelig
- Lauftext-Geschwindigkeit einstellbar (1–10)
- Web-Dashboard in Kategorien (Tabs) mit Speichern-Leiste
- PC-Monitor und Dino-Spiel entfernt

## v1.1

- Dimmen mit Tiefdimmung, Ruhemodus nach Zeitplan, stehender Text, Sonderzeichen, Fix für Doppeltipp/Info-Folien/Lauftext (Zeitüberlauf)

## v1.0

- Erste Version: animierte Augen, Touch-Gesten, Info-Folien, Wetter, Lauftext, WLAN-Setup-Portal, Web-Dashboard, HTTP-API
