<p align="right">
  <a href="README.de.md">🇩🇪 Deutsch</a> |
  <a href="README.md">🇬🇧 English</a> |
  <a href="README.es.md">🇪🇸 Español</a> |
  <a href="README.fr.md">🇫🇷 Français</a> |
  <a href="README.it.md">🇮🇹 Italiano</a>
</p>

# MeshCore Web Dashboard

<sub>by DJ0ABR (c) 2026</sub>

Web-Dashboard für **MeshCore Nodes** mit Fokus auf Desktop-Systeme.

-   läuft unter **Linux** (Raspberry Pi, PC usw.)
-   **kein Smartphone erforderlich**
-   optimiert für **Desktop-Monitore**
-   und zusätzlich für **Mobilgeräte**
-   Zugriff mit **jedem modernen Browser** im lokalen Netzwerk

------------------------------------------------------------------------

# Funktionen

Das Dashboard bietet unter anderem:

-   Anzeige von **Nodes und Rooms**
-   **Chat-Nachrichten**
-   **beliebige Anzahl an Nodes (kein Limit!)**
-   **Kanalverwaltung**
-   **Repeater-Discovery**
-   **Kartenansicht** von Nodes

------------------------------------------------------------------------

## Screenshots

<p align="center">
<img src="doku/pic1.png" width="250">
<img src="doku/pic2.png" width="250">
<img src="doku/pic3.png" width="250">
</p>

<p align="center">
<img src="doku/pic4.png" width="250">
<img src="doku/pic5.png" width="250">
<img src="doku/pic6.png" width="250">
</p>

------------------------------------------------------------------------

# Hardware vorbereiten

1. Öffne auf der MeshCore Webseite den **WebFlasher**.
2. Flashe das Paket `Companion-USB`.

------------------------------------------------------------------------

# Installation

Repository klonen:

``` bash
git clone https://github.com/dj0abr/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Software installieren:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Programm starten

Hardware über USB anschließen.

Seriellen Port ermitteln:

``` bash
ls /dev/tty*
```

Beispiele:

Wenn der Port **ttyUSB0** ist:

``` bash
./meshcore_api
```

Wenn der Port **ttyACM0** ist:

``` bash
./meshcore_api /dev/ttyACM0
```

------------------------------------------------------------------------

# Dashboard öffnen

Im Browser innerhalb des Heimnetzwerks:

    http://IP_des_Raspi

------------------------------------------------------------------------

# Erste Einrichtung

1.  Rechts oben auf das **Zahnrad-Symbol** klicken.
2.  Im Setup-Fenster folgende Daten eingeben:

-   **Name** (Leerzeichen und Sonderzeichen erlaubt)
-   **Längengrad**
-   **Breitengrad**

3.  Auf **Apply** klicken.

------------------------------------------------------------------------

# Betrieb

Das Dashboard ist jetzt einsatzbereit.

Wenn Stationen empfangen werden, erscheinen diese in der **Node-Liste
auf der linken Seite**.

⚠️ Hinweis:  
Bis erste Nodes erscheinen, kann **eine Stunde oder länger** vergehen.

## Bedienung

- **Rechtsklick auf einen Node:**  
  Es öffnet sich eine **Karte mit der Position** des Nodes.

- **Linksklick auf einen Chat oder Room:**  
  Das **Chatfenster wird geöffnet**.

------------------------------------------------------------------------

# Repeater Discovery

Um MeshCore-Repeater zu suchen:

1.  Rechts oben auf das **Lupe-Symbol** klicken.
2.  Das Fenster **Repeater Discovery** öffnet sich.
3.  Auf **START** klicken.

Es werden erreichbare Repeater gesucht.\
Der Vorgang kann **mehrfach ausgeführt** werden.

------------------------------------------------------------------------

## TCP-Verbindung (MeshCore Companion)

Neben der klassischen USB-Verbindung kann das Backend auch über TCP mit einem MeshCore Companion kommunizieren.

### Voraussetzung

Der Companion muss mit einem speziellen Image geflasht werden:
https://www.weyhmueller.org/webtools/esp32_ssid_patcher.html

### Companion flashen

1. Companion anschließen  
2. Webseite öffnen  
3. Firmware auswählen und flashen  
4. Gerät neu starten  

### IP-Adresse herausfinden

Nach einem Reset wird die IP-Adresse auf dem Display angezeigt.  
Ohne Display: im Router (DHCP-Liste) nachsehen.

### Verbindung herstellen

```bash
./meshcore_api tcp://<IP>:5000
```

Beispiel:

```bash
./meshcore_api tcp://192.168.10.170:5000
```

------------------------------------------------------------------------

## Blacklist

Im Backend-Verzeichnis kann eine UTF-8 Textdatei mit dem Namen `blacklist.txt` abgelegt werden.

Wenn ein Node-Name in dieser Datei enthalten ist, wird der Node ignoriert und nicht in die Datenbank geschrieben.

- ein Node-Name pro Zeile
- UTF-8 / Emoji unterstützt
- Zeilen mit `#` am Anfang werden ignoriert
- Blacklist-Repeaternodes werden beim Start aus `repeaternodes` entfernt

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
