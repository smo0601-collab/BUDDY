# PCBuddy – HTTP-API und Seriell-Befehle

Alle Aktionen funktionieren per **GET oder POST**. Antwort ist JSON, meist `{"ok":true}`.
Basis-Adresse: `http://pcbuddy.local` (oder die IP-Adresse). CORS ist freigegeben, die Aufrufe gehen also auch aus
eigenen Webseiten.

## Gesicht & Gesten

| Endpunkt | Parameter | Wirkung |
|---|---|---|
| `/api/emotion` | `name`, `ms` (Standard 10000, `0` = dauerhaft) | Emotion anzeigen |
| `/api/tap` | – | wie einmal Tippen |
| `/api/double` | – | wie Doppeltipp (Info-Folien an/aus) |
| `/api/long` | – | wie langes Drücken (verliebt) |
| `/api/shake` | – | wie Schütteln (wütend) |
| `/api/rock` | – | wie langsames Schaukeln („Uh“) |
| `/api/sleep` · `/api/wake` | – | Schlafmodus an / aufwecken |
| `/api/cycle` | `mode=run\|idle\|off`, `sec=2…60`, `order=seq\|random` | automatischer Emotions-Wechsel |

**Emotionen (`name`):**
`neutral`, `cute`, `laugh`, `love`, `heartbeat`, `anime_love`, `mini_love`, `kissou`, `star`, `surprised`, `confused`,
`dizzy`, `serious`, `angry`, `fire`, `sad`, `crying`, `sleepy`, `left`, `right`, `uh`

Diese Kurzformen gehen auch: `happy`/`smile` → cute · `kiss` → kissou · `herz`/`beat` → heartbeat ·
`wow` → surprised · `lol` → laugh · `stars` → star · `huh` → confused · `woozy` → uh · `tired` → sleepy

## Anzeige

| Endpunkt | Parameter | Wirkung |
|---|---|---|
| `/api/text` | `text`, `mode=once\|loop\|static`, `speed=1…10`, `ms` (bei static, Standard 15000, `0` = bis Tippen) | Text anzeigen |
| `/api/notify` | `text` | Ton + Vibration + Lauftext |
| `/api/info` | – | Info-Folien öffnen |
| `/api/clock` | `style=0…7` (optional), `save=1` | Uhr zeigen (bis Doppeltipp). Mit `style` als Vorschau in diesem Design, mit `save=1` dauerhaft. |
| `/api/clock/demo` | `style=0…7` (optional) | Ziffern-Wechsel sofort vorführen |

**Uhr-Designs (`style`):** `0` Pac-Man, `1` Fallende Blöcke, `2` Schlange, `3` Raumschiff, `4` Katze, `5` Klappzahlen,
`6` Tischtennis, `7` Wechselnd (jede Stunde ein anderes)
| `/api/dim` | `level=1…100`, `save=1` | Helligkeit in % (im Ruhemodus: Ruhemodus-Helligkeit) |
| `/api/quiet` | `state=on\|off\|auto\|toggle` | Ruhemodus (`auto` = nach Zeitplan) |
| `/api/beep` | `f` (Hz), `ms` | Ton |
| `/api/vibrate` | `ms` | Vibration |

`/api/marquee` ist ein Alias für `/api/text` und bleibt aus Kompatibilitätsgründen erhalten.

## Wecker & Termine

| Endpunkt | Parameter | Wirkung |
|---|---|---|
| `/api/alarms` | – | alle Wecker und Termine mit nächstem Termin (`next`), Melodienliste, Klingel-Zustand |
| `/api/alarm/set` | `i` (fehlt oder `-1` = neu), `time=HH:MM`, `days`, `label`, `emo`, `snd`, `vibe`, `snooze`, `en` | Wecker anlegen oder ändern (nur mitgeschickte Felder) |
| `/api/alarm/del` | `i` | Wecker löschen |
| `/api/event/set` | `i`, `date=JJJJ-MM-TT` (oder `TT.MM.JJJJ`), `time`, `rep`, `lead`, `label`, `emo`, `snd`, `vibe`, `en` | Termin anlegen oder ändern |
| `/api/event/del` | `i` | Termin löschen |
| `/api/alarm/stop` · `/api/alarm/snooze` | – | klingelnden Wecker/Termin ausschalten bzw. schlummern |
| `/api/alarm/test` · `/api/event/test` | `i` und/oder dieselben Felder wie bei `…/set` | Probelauf sofort. Mit `i` den gespeicherten Eintrag, mitgeschickte Felder gelten nur für den Test. Ohne `i` einen neuen Eintrag testen. Es wird nichts gespeichert; der Test endet nach 30 s oder per `/api/alarm/stop`. |
| `/api/sound` | `id=0…10` oder `stop=1` | Melodie probehören |

- **`days`**: Bitmaske der Wochentage, Mo = 1, Di = 2, Mi = 4, Do = 8, Fr = 16, Sa = 32, So = 64
  (Mo–Fr = `31`, Wochenende = `96`, täglich = `127`). `0` = einmalig: klingelt beim nächsten Mal und schaltet sich danach aus.
- **`rep`**: `0` einmalig, `1` täglich, `2` wöchentlich, `3` monatlich, `4` jährlich. Bei Wiederholungen gilt das Datum als Start
  (bei Geburtstagen darf es das Geburtsjahr sein).
- **`lead`**: so viele Minuten vorher erinnern (0–240).
- **`emo`**: Name der Emotion (siehe oben) oder ihre Nummer. **`snooze`**: Minuten (0 = kein Schlummern).
- **`snd`**: `0` Aus, `1` Piep, `2` Doppelpiep, `3` Klassischer Wecker, `4` Gong, `5` Fanfare, `6` Fröhlich, `7` Sanft,
  `8` Happy Birthday, `9` Für Elise, `10` Arcade.
- Maximal 8 Wecker und 20 Termine. Name: Wecker bis 23, Termine bis 33 Byte (Umlaute zählen doppelt).

## Status & Einstellungen

| Endpunkt | Methode | Wirkung |
|---|---|---|
| `/api/status` | GET | aktueller Zustand (siehe unten) |
| `/api/settings` | GET | alle Einstellungen als JSON |
| `/api/settings` | POST (Formular) | Einstellungen ändern. Nur die mitgeschickten Felder werden geändert. |
| `/api/sensor` | GET | Live-Werte: Neigungskontakt, Wechsel/s, Touch |
| `/api/scan` | GET | WLAN-Netze suchen (mehrfach abfragen, bis `status: done`) |
| `/api/wifi` | POST | `ssid`, `pass`, `host`, `oled` speichern und neu starten |

**`/api/status` liefert:** `name`, `fw`, `host`, `ip`, `ssid`, `rssi`, `mode`, `emotion`, `portal`, `quiet`,
`quiet_mode`, `cycle`, `bright`, `oled`, `uptime`, `heap`, `ota`, `part`, `upd_pw`, `wifi_reconnects`, `wifi_reason`,
`city`, `ringing` (`alarm`, `event` oder leer), `ring_test`, `next_alarm` (z. B. `morgen 07:30`), `time`, `wx` (`temp`, `code`, `hum`, `wind`, `text`)

**Felder für `/api/settings`:**

| Feld | Bedeutung |
|---|---|
| `host` | Gerätename (`host.local`) |
| `oled` | `0` = 0,96" SSD1306, `1` = 1,3" SH1106 (Neustart) |
| `flip` | Display um 180° drehen |
| `bright` | Helligkeit normal (1–100) |
| `quiet_on`, `quiet_from`, `quiet_to` | Ruhemodus-Zeitplan (`HH:MM`) |
| `quiet_bright`, `quiet_disp`, `quiet_mute`, `quiet_novibe` | Ruhemodus: Helligkeit, Anzeige (0 Gesicht, 1 Schlaf, 2 Uhr, 3 aus), Ton aus, Vibration aus |
| `city`, `lat`, `lon`, `tz` | Ort fürs Wetter, Zeitzone (POSIX-String) |
| `sl_weather`, `sl_clock`, `sl_events`, `sl_wifi`, `sl_ip`, `sl_fw`, `lock_info` | Info-Folien, Doppeltipp sperren |
| `alarm_max`, `ev_quiet` | Wecker klingelt höchstens so viele Minuten (1–60) · Termine im Ruhemodus mit Ton |
| `sound`, `vibe`, `pac_sound` | Ton, Vibration, Uhr-Geräusche zur vollen Stunde |
| `clock_style` | Uhr-Design 0–7 (siehe oben) |
| `shake_on`, `shake_sens` | Neigungssensor an/aus, Empfindlichkeit 1–5 |
| `cycle_mode`, `cycle_order`, `cycle_sec`, `emo_mask` | Auto-Wechsel (0 aus, 1 Durchlauf, 2 Leerlauf), Reihenfolge, Sekunden, Bitmaske der Emotionen |
| `mq_speed` | Lauftext-Geschwindigkeit 1–10 |
| `sleep_min` | Auto-Schlaf nach Minuten (0 = nie) |
| `upd_pw` | Update-Passwort (leer = keins) |

## System

| Endpunkt | Wirkung |
|---|---|
| `/api/update` (POST, multipart, `?size=…&pw=…`) | Firmware-Update hochladen (`.bin`) |
| `/api/restart` | Neustart |
| `/api/portal` | WLAN-Setup-Portal öffnen |
| `/api/reset` | **Werkseinstellungen** (löscht WLAN und alle Einstellungen) |

## USB-Seriell (115200 Baud, Zeilenende `\n`)

```
emotion <name> [ms]     text <t>        loop <t>        show <t>        speed <1-10>
notify <t>              beep [f]        vibe [ms]       dim <1-100>     quiet on|off|auto|toggle
clock [0-7] [demo]            cycle on|idle|off               tap   double   long   shake   rock
info   sleep   wake   portal   restart   status   help
alarm                   (Liste)         alarm stop      alarm snooze    alarm test <i>    alarm event <i>
sound <0-10>            Melodie probehören
```

## Beispiele

```bash
# Benachrichtigung, wenn ein langer Befehl fertig ist
make && curl -s "http://pcbuddy.local/api/notify?text=Build%20fertig"

# Ruhemodus jeden Abend per Cron
0 22 * * *  curl -s "http://pcbuddy.local/api/quiet?state=on"
```

```bash
# Wecker Mo–Fr um 6:45 mit sanfter Melodie und fröhlichem Gesicht anlegen
curl -s "http://pcbuddy.local/api/alarm/set?time=06:45&days=31&label=Aufstehen&snd=7&emo=cute"

# Geburtstag jedes Jahr am 14. März um 9 Uhr mit Happy Birthday und pochendem Herz
curl -s "http://pcbuddy.local/api/event/set?date=1956-03-14&time=09:00&rep=4&label=Geburtstag%20Oma&snd=8&emo=heartbeat"
```

```bash
# Probelauf eines neuen Weckers, ohne ihn zu speichern
curl -s "http://pcbuddy.local/api/alarm/test?time=06:45&label=Probe&snd=8&emo=heartbeat"
```

```powershell
# Windows: Text anzeigen
Invoke-RestMethod "http://pcbuddy.local/api/text?mode=static&text=Meeting%20bis%2015%20Uhr&ms=0"
```

```yaml
# Home Assistant (configuration.yaml)
rest_command:
  pcbuddy_notify:
    url: "http://pcbuddy.local/api/notify?text={{ text | urlencode }}"
```
