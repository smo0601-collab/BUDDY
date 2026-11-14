# Changelog

Alle Versionen von PCBuddy. Die neueste steht oben.

## v1.8.0

- **6 neue Uhr-Designs** neben der Pac-Man-Uhr, alle mit eigener Animation zum Minutenwechsel:
  - **Fallende Blöcke** – alte Blöcke fallen herunter, neue stapeln sich auf, unten wird eine Reihe abgeräumt
  - **Schlange** – jagt Äpfel am Rand, frisst die alte Ziffer und zeichnet die neue Punkt für Punkt
  - **Raumschiff** – schießt die alte Ziffer ab, die neue wird hereingebeamt; Sterne und Untertasse
  - **Katze** – schubst die alte Ziffer vom Tisch, die neue hüpft herunter
  - **Klappzahlen** – Fallblatt-Uhr mit Datum und Sekundenbalken
  - **Tischtennis** – Ping-Pong mit der Uhrzeit als Spielstand, zum Minutenwechsel verpasst ein Schläger den Ball
  - **Wechselnd** – jede Stunde ein anderes Design
- Die neue Ziffer erscheint bei allen Designs genau zur vollen Minute. Das Design gilt für den Uhr-Bildschirm, die Info-Folie und die Nacht-Uhr im Ruhemodus.
- Dashboard: Karte **Anzeige → Uhr** mit Design-Auswahl, Beschreibung und „Auf dem Display vorführen“ (Vorschau ohne Speichern).
- „Waka-Waka“ heißt jetzt **Geräusche zur vollen Stunde** und gilt für alle Designs (Miau, Laser, Klack …).
- API: `/api/clock?style=0…7` (Vorschau, mit `&save=1` speichern), `/api/clock/demo?style=…`, Einstellung `clock_style`; Seriell `clock <0-7> [demo]`.

## v1.7.1

- **Test-Funktion für Wecker und Termine** ausgebaut: In der Liste hat jeder Eintrag einen **▶ Test**-Knopf.
  Im Editor testet **Testen** die gerade eingestellten Werte (Uhrzeit, Name, Gesicht, Melodie …), auch für neue,
  noch nicht gespeicherte Einträge, ohne etwas zu speichern. Während des Tests wird der Knopf zu „Test beenden“.
- Auf dem Display steht beim Test „TEST“ bzw. „Termin (Test)“; der Termin-Test zeigt die echte Erinnerung (z. B. „in 30 min · 15:00“).
- Ein Test endet nach 30 s von selbst und ändert nichts am gespeicherten Eintrag (einmalige Wecker bleiben an, kein Schlummern).
- API: `/api/alarm/test` und `/api/event/test` nehmen dieselben Felder wie `…/set`; ohne `i` wird ein neuer Eintrag getestet.
  `ring_test` in `/api/status`. Seriell: `alarm event <i>` testet einen Termin.

## v1.7.0

- **Wecker** (bis zu 8): Uhrzeit, Wochentage (oder einmalig), Name, eigenes Gesicht, eigene Melodie, Vibration, Schlummern (5/10/15 min).
  Klingelt auch im Ruhemodus mit hellem Display. Am Gerät: tippen oder schütteln = schlummern, 2× tippen oder lang drücken = aus,
  danach „Guten Morgen!“ mit dem Wetter. Hört nach der eingestellten Klingeldauer von selbst auf.
- **Termine** (bis zu 20): Datum, Uhrzeit, Name, Wiederholung (einmalig, täglich, wöchentlich, monatlich, jährlich, z. B. Geburtstage),
  Erinnerung 5 min bis 4 h vorher, eigenes Gesicht, eigene Melodie, Vibration. Im Ruhemodus wahlweise nur Anzeige.
- **10 Melodien** zum Probehören: Piep, Doppelpiep, klassischer Wecker, Gong, Fanfare, Fröhlich, Sanft, Arcade, Happy Birthday, Für Elise.
- Neuer Dashboard-Tab **„Wecker & Termine“** mit Listen, Editoren, Ein/Aus-Schaltern und Test-Knopf; Klingel-Leiste mit
  Schlummern/Aus auf jeder Seite; „Nächster Wecker“ im Status.
- Neue Info-Folie **„Wecker & Termine“** mit den nächsten drei Einträgen.
- API: `/api/alarms`, `/api/alarm/set|del|test|stop|snooze`, `/api/event/set|del|test`, `/api/sound`; Seriell: `alarm`, `sound`.

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
