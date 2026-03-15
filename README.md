# MeshCore Web Dashboard

Web-Dashboard für **MeshCore Nodes** mit Fokus auf Desktop-Systeme.

-   läuft unter **Linux** (Raspberry Pi, PC usw.)
-   **kein Smartphone erforderlich**
-   optimiert für **Desktop-Monitore**
-   Zugriff mit **jedem modernen Browser** im lokalen Netzwerk

------------------------------------------------------------------------

# Funktionen

Das Dashboard bietet unter anderem:

-   Anzeige von **Nodes und Rooms**
-   **Chat-Nachrichten**
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
</p>

------------------------------------------------------------------------

# Hardware vorbereiten

1. Öffne auf der MeshCore Webseite den **WebFlasher**.
2. Flashe das Paket `Companion-USB`.

------------------------------------------------------------------------

# Installation

Repository klonen:

``` bash
git clone https://github.com/USER/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Pakete installieren:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Datenbank vorbereiten

MariaDB starten:

``` bash
sudo mariadb
```

Folgenden SQL-Block einfügen:

``` sql
CREATE DATABASE IF NOT EXISTS meshcore
CHARACTER SET utf8mb4
COLLATE utf8mb4_general_ci;

CREATE USER IF NOT EXISTS 'meshcore'@'localhost' IDENTIFIED BY '';

GRANT ALL PRIVILEGES ON meshcore.* TO 'meshcore'@'localhost';

FLUSH PRIVILEGES;
```

------------------------------------------------------------------------

# Software bauen

``` bash
make
```

HTML-Dateien installieren:

``` bash
sudo cp -R html/* /var/www/html
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

⚠️ Hinweis:\
Bis erste Nodes erscheinen, kann **eine Stunde oder länger** vergehen.

------------------------------------------------------------------------

# Repeater Discovery

Um MeshCore-Repeater zu suchen:

1.  Rechts oben auf das **Lupe-Symbol** klicken.
2.  Das Fenster **Repeater Discovery** öffnet sich.
3.  Auf **START** klicken.

Es werden erreichbare Repeater gesucht.\
Der Vorgang kann **mehrfach ausgeführt** werden.
