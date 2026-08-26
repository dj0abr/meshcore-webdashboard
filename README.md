<p align="right">
  <a href="README.de.md">🇩🇪 Deutsch</a> |
  <a href="README.md">🇬🇧 English</a> |
  <a href="README.es.md">🇪🇸 Español</a> |
  <a href="README.fr.md">🇫🇷 Français</a> |
  <a href="README.it.md">🇮🇹 Italiano</a>
</p>

# MeshCore Web Dashboard

<sub>by DJ0ABR (c) 2026</sub>

Web dashboard for **MeshCore nodes** with a focus on desktop systems.

-   runs on **Linux** (Raspberry Pi, PC, etc.)
-   **no smartphone required**
-   optimized for **desktop monitors**
-   and additionally for **mobile devices**
-   accessible with **any modern browser** on the local network

------------------------------------------------------------------------

# Features

Among other things, the dashboard offers:

-   display of **nodes and rooms**
-   **chat messages**
-   **any number of nodes (no limit!)**
-   **channel management**
-   **repeater discovery**
-   **map view** of nodes

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

# Prepare the hardware

1. Open the **WebFlasher** on the MeshCore website.
2. Flash the `Companion-USB` package.

------------------------------------------------------------------------

# Installation

Clone the repository:

``` bash
git clone https://github.com/dj0abr/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Install the software:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Start the program

Connect the hardware via USB.

Determine the serial port:

``` bash
ls /dev/tty*
```

Examples:

If the port is **ttyUSB0**:

``` bash
./meshcore_api
```

If the port is **ttyACM0**:

``` bash
./meshcore_api /dev/ttyACM0
```

------------------------------------------------------------------------

# Open the dashboard

In a browser within the home network:

    http://IP_of_the_Raspberry_Pi

------------------------------------------------------------------------

# Initial setup

1.  Click the **gear icon** in the upper right corner.
2.  In the setup window, enter the following data:

-   **Name** (spaces and special characters are allowed)
-   **Longitude**
-   **Latitude**

3.  Click **Apply**.

------------------------------------------------------------------------

# Operation

The dashboard is now ready to use.

When stations are received, they appear in the **node list on the left
side**.

⚠️ Note:  
It may take **an hour or longer** before the first nodes appear.

## Usage

- **Right-click on a node:**  
  A **map with the node's position** opens.

- **Left-click on a chat or room:**  
  The **chat window opens**.

------------------------------------------------------------------------

# Repeater Discovery

To search for MeshCore repeaters:

1.  Click the **magnifying glass icon** in the upper right corner.
2.  The **Repeater Discovery** window opens.
3.  Click **START**.

Reachable repeaters will be searched for.\
The process can be **run multiple times**.

------------------------------------------------------------------------

## Home Repeater Query

In addition to Repeater Discovery, a **Home Repeater** can be queried directly.

- enter the Home Repeater name and an optional admin password
- login attempts and individual query pages are retried automatically if RF transmission fails
- the complete neighbour list is loaded page by page instead of returning only the first subset
- repeaters are matched against `repeaternodes` using the first **8 characters of the public key**
- available coordinates are added and the distance from the Home Repeater is calculated
- obviously implausible positions more than **400 km** away are ignored for display
- the result table can be sorted by `#`, pubkey, name, "heard ago", SNR and distance and can be copied to the clipboard
- repeaters with valid coordinates can also be displayed on an **OpenStreetMap map** with connection lines to the Home Repeater

The password is used only for the current query and is not stored permanently as a repeater password.

------------------------------------------------------------------------

## TCP Connection (MeshCore Companion)

In addition to the USB connection, the backend can also connect to a MeshCore Companion via TCP.

### Requirement

The Companion must be flashed with a special image:
https://www.weyhmueller.org/webtools/esp32_ssid_patcher.html

### Flashing the Companion

1. Connect the Companion  
2. Open the website  
3. Select and flash firmware  
4. Reboot the device  

### Getting the IP address

After a reset, the IP address is shown on the device display.  
If there is no display, check your router (DHCP list).

### Connecting

```bash
./meshcore_api tcp://<IP>:5000
```

Example:

```bash
./meshcore_api tcp://192.168.10.170:5000
```

------------------------------------------------------------------------

## Blacklist

A UTF-8 text file named `blacklist.txt` can be placed in the backend directory.

If a node name matches an entry in this file, the node will be ignored and not written to the database.

- one node name per line
- UTF-8 / emoji supported
- lines starting with `#` are ignored
- blacklisted repeater nodes are removed from `repeaternodes` on backend startup

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
