# Telefon-Geocache

Ein Geocache, der in einem alten Telefon steckt. Wer den Cache findet, nimmt den
Hörer ab und wird per Sprachansage und Display durch ein kleines Rätsel geführt:
Handynummer eingeben, Münze einwerfen – das Telefon ruft daraufhin **zurück** und
verrät über die angezeigte Rufnummer einen PIN. Wird der PIN korrekt eingegeben,
öffnet ein Relais die Klappe zum Logbuch.

[![Demo-Video](http://img.youtube.com/vi/i5rzEiwlTfw/0.jpg)](http://www.youtube.com/watch?v=i5rzEiwlTfw "Telefon-Geocache Demo")

## Spielablauf

Die Firmware ist eine Zustandsmaschine (`step0`–`step7` in `cache/cache.ino`):

| Schritt | Anzeige / Aktion |
|---|---|
| 0 | „Hörer ans Ohr, drücke Taste 5" – GSM-Modul wird initialisiert |
| 1 | Begrüßung; Netz wird gesucht und die Uhrzeit geprüft (nur **8–22 Uhr**, sonst Schritt 7) |
| 2 | Handynummer über das Tastenfeld eingeben, Abschluss mit `#` |
| 3 | „Münze einwerfen" – Wartet auf den Münzeinwurf (Lichtschranke) |
| 4 | Server abfragen (HTTP), Absender-Rufnummer setzen lassen, dann den Spieler **anrufen** (5 s klingeln, NICHT abheben) |
| 5 | Die letzten 4 Ziffern der angezeigten Rufnummer eingeben |
| 6 | Korrekt → Relais öffnet die Klappe; falsch → Hinweis, neuer Versuch |
| 7 | Außerhalb der Öffnungszeiten → Gerät geht in den Tiefschlaf |

Ein 5-Minuten-Countdown läuft global mit; nach Ablauf legt sich das Gerät schlafen.

## So funktioniert der „Rückruf-PIN"

Der eigentliche Trick: Der Spieler muss nicht abnehmen, sondern nur die **angezeigte
Rufnummer** ablesen. Deren letzte 4 Ziffern sind der PIN.

```
Arduino (cache.ino)                        Server (index.php)            sipgate
  │  HTTP GET (über GPRS) ───────────────────▶                              
  │                                          │  PUT /devices/{id}/callerid │
  │                                          │   = +491579999<code>  ──────▶
  │  ◀──────────────── Antwort: <code> ──────┘                              
  │  GPRS trennen                                                           
  │  SIM800L wählt die Handynummer (ATD) ─────────────────────────────────▶ Telefon des
  │  5 s warten, dann auflegen (ATH)                                         Spielers klingelt
  │                                                                          mit +491579999<code>
```

Der Anruf wird seit der Umstellung auf einen **sipgate-neo-PBX-Account** direkt vom
SIM800L aufgebaut (nicht mehr über die sipgate-Click-to-Dial-API – neo erlaubt kein
Nummer-zu-Nummer-Dialing mehr). Der Server setzt nur noch die angezeigte
Absender-Rufnummer und liefert den 4-stelligen Code zurück.

## Projektstruktur

| Pfad | Inhalt |
|---|---|
| `cache/` | Arduino-Firmware (`cache.ino`) + Build-Anleitung (`BUILD.md`) |
| `lib/` | Alle Arduino-Bibliotheken, vendored (siehe unten) |
| `platformio.ini` | PlatformIO-Build-Konfiguration |
| `web-call-starter/` | PHP-Server: setzt Absender-Rufnummer & liefert PIN-Code |
| `mp3/` | Sprachansagen + Skripte zur TTS-Erzeugung (Google Cloud, Azure, AWS Polly) |
| `pcb/` | EAGLE-Schaltplan und Platinen-Layout |
| `experimente/` | Ältere Test-Sketche |

## Hardware

- **Arduino Mega 2560** (Steuerung)
- **SIM800L** GSM-Modul mit sipgate-SIM (Daten + Sprachanruf)
- **DFPlayer Mini** MP3-Modul + Lautsprecher (Sprachansagen)
- **LCD 20×4** mit I²C-Backpack
- **3×4-Tastenfeld** (Matrix)
- **Lichtschranke** als Münzeinwurf-Sensor
- **Relais** SRD-05VDC-SL-C (öffnet die Klappe)
- **Step-Up-Wandler** für die Stromversorgung des SIM800L

### Verdrahtung (Arduino Mega)

| Komponente | Pins |
|---|---|
| Tastenfeld | Zeilen 34, 35, 36, 37 · Spalten 31, 32, 33 |
| Relais | D10 |
| Münzeinwurf (analog) | A13 |
| SIM800L (UART) | `Serial1`: RX1 = D19, TX1 = D18 · 19200 Baud |
| DFPlayer Mini (UART) | `Serial3`: RX3 = D15, TX3 = D14 |
| LCD (I²C) | SDA = D20, SCL = D21 · Adresse `0x27` |

Die GSM-PIN ist in `cache/cache.ino` definiert (`GSM_PIN`).

## Firmware bauen

Der Build läuft reproduzierbar über [PlatformIO](https://platformio.org/) – alle
Details in **[`cache/BUILD.md`](cache/BUILD.md)**. Kurzform (im Repo-Wurzelverzeichnis):

```bash
pio run                 # kompilieren
pio run -t upload       # auf den Arduino Mega 2560 flashen
pio device monitor      # serieller Monitor (115200 Baud)
```

### Bibliotheken (vendored)

Alle Bibliotheken liegen mit fester Version in `lib/`, damit der Build dauerhaft
und ohne Abhängigkeit von der PlatformIO-Registry funktioniert:

| Library | Version | Anmerkung |
|---|---|---|
| TinyGSM | 0.12.0 | GSM/GPRS + Sprachanruf (SIM800) |
| ArduinoHttpClient | 0.6.1 | HTTP-Request an den Server |
| Keypad | 3.1.1 | Tastenfeld |
| DFMiniMp3 | 1.0.5 | bewusst alte Version – ab 1.0.6 inkompatible Callback-API |
| LiquidCrystal_I2C | fdebrabander | LCD-Ansteuerung über I²C |

## Server (`web-call-starter`)

Kleiner PHP-Endpunkt, der bei jedem Aufruf eine zufällige 4-stellige Zahl erzeugt,
diese als Absender-Rufnummer am sipgate-Gerät setzt und den Code als Antwort
zurückgibt.

- **`SIPGATE_CREDENTIALS`** muss als Umgebungsvariable gesetzt sein
  (Base64 von `tokenId:token` des sipgate-Accounts).
- `$deviceId` in `index.php` an das eigene sipgate-Gerät anpassen (neo nutzt oft `e0`/`p0`).
- Abhängigkeiten via Composer installieren: `composer install`
  (die `vendor/autoload.php` wird in `index.php` eingebunden).
- `telefoncache.vhost` enthält eine Beispiel-Apache-vHost-Konfiguration.

> Hinweis: Der eigentliche Anruf erfolgt nicht mehr serverseitig, sondern direkt
> vom Arduino (siehe oben).

## Sprachansagen (`mp3`)

Die Ansagen `0001.mp3`–`0008.mp3` entsprechen den Spielschritten und werden vom
DFPlayer Mini abgespielt. Sie lassen sich per Text-to-Speech neu erzeugen; im
Ordner `mp3/` liegen Skripte für **Google Cloud TTS**, **Azure** und **AWS Polly**.
Die zugehörigen Texte stehen in `mp3/texte/`.

## Platine (`pcb`)

EAGLE-Dateien (`*.sch` Schaltplan, `*.brd` Layout) inklusive gerenderter
Vorschaubilder und PDF-Export.
