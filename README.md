## telefon-geocache

### Idee

Hörer abheben 
-> Ist die aktuelle Uhrzeit zwischen 8 und 22 Uhr? (RTC)
--> Nein? --> Displayanzeige "Dieses Gerät ist nur zwischen 8 und 22 Uhr verfügbar."

-> Arduino start -> GSM start
-> Displayanzeige "Hallo"
-> Sound abspielen "Hallo, willkommen zu meinem Cache. Du hast 5 Minuten Zeit und nur einen Versuch."
-> Display zeigt 5-Minuten-Zähler an

-> Displayanzeige & Sound: "Gib deine Handynummer ein und drücke die Rautetaste."

-> Zahleneingabe
-> Raute gedrückt?

-> Displayanzeige "Münze einwerfen"
-> Sound abspielen "Wirf eine Münze ein, damit das Spiel gestartet wird."

-> Münze eingeworfen? (Lichtschranke)
-> Displayanzeige & Sound: "Danke"

-> GSM bereit?
-> Verbindungsaufbau mit spezieller Rufnummer (z.B. 0211-1234-1337)
-> Klingeln?
-> Auflegen.

-> Displayanzeige & Sound: "Gib die letzten 4 Stellen des Anrufers ein."
-> Richtig?
-> Klappe unten öffnet (Relais)
-> Displayanzeige & Sound: "Trag dich ein, schließ die Klappe und häng auf."

### Benötigte Elektronik:
- Arduino 
- MP3-Player (2)
- RTC	(2)
- Display	(2)
- GSM-Modul	(3)
- Tastatur	(7)
- Lichtschranke (1)
- Relais	(1)
- (Lautsprecher)
