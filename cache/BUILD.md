# Build (PlatformIO)

Der Sketch wird reproduzierbar mit [PlatformIO](https://platformio.org/) gebaut.
Alle Bibliotheken sind in `platformio.ini` auf feste Versionen gepinnt; die
LCD-Bibliothek liegt vendored in `lib/LiquidCrystal_I2C/`.

## Einmalig: PlatformIO installieren

```bash
# als CLI (empfohlen)
pip install -U platformio
# oder: VS Code + PlatformIO IDE Extension
```

## Bauen / Flashen

Alle Befehle im Repo-Wurzelverzeichnis (dort liegt `platformio.ini`):

```bash
pio run                 # kompilieren
pio run -t upload       # auf den Arduino Mega 2560 flashen
pio device monitor      # serieller Monitor (115200 Baud)
```

Beim ersten `pio run` lädt PlatformIO automatisch die AVR-Toolchain und die
gepinnten Bibliotheken herunter (Internet nötig). Danach ist der Build offline
reproduzierbar.

## Board

- Arduino Mega 2560 (`board = megaatmega2560`)

## Bibliotheken

| Library | Version |
|---|---|
| makuna/DFMiniMp3 | 1.0.5 (letzte Version mit der hier genutzten Callback-API) |
| chris--a/Keypad | 3.1.1 |
| vshymanskyy/TinyGSM | 0.12.0 |
| arduino-libraries/ArduinoHttpClient | 0.6.1 |
| LiquidCrystal_I2C (fdebrabander) | vendored in `lib/` |
