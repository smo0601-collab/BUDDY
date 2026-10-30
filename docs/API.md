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
| `/api/clock` | – | Pac-Man-Uhr zeigen (bis Doppeltipp) |
| `/api/clock/demo` | – | Fress-Animation sofort vorführen |
| `/api/dim` | `level=1…100`, `save=1` | Helligkeit in % (im Ruhemodus: Ruhemodus-Helligkeit) |
| `/api/quiet` | `state=on\|off\|auto\|toggle` | Ruhemodus (`auto` = nach Zeitplan) |
| `/api/beep` | `f` (Hz), `ms` | Ton |
| `/api/vibrate` | `ms` | Vibration |

`/api/marquee` ist ein Alias für `/api/text` und bleibt aus Kompatibilitätsgründen erhalten.

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
`city`, `time`, `wx` (`temp`, `code`, `hum`, `wind`, `text`)

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
| `sl_weather`, `sl_clock`, `sl_wifi`, `sl_ip`, `sl_fw`, `lock_info` | Info-Folien, Doppeltipp sperren |
| `sound`, `vibe`, `pac_sound` | Ton, Vibration, Waka-Waka zur vollen Stunde |
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
clock [demo]            cycle on|idle|off               tap   double   long   shake   rock
info   sleep   wake   portal   restart   status   help
```

## Beispiele

```bash
# Benachrichtigung, wenn ein langer Befehl fertig ist
make && curl -s "http://pcbuddy.local/api/notify?text=Build%20fertig"

# Ruhemodus jeden Abend per Cron
0 22 * * *  curl -s "http://pcbuddy.local/api/quiet?state=on"
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
