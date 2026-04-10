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
-   accessible with **any modern browser** on the local network

------------------------------------------------------------------------

# Features

Among other things, the dashboard offers:

-   display of **nodes and rooms**
-   **chat messages**
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
git clone https://github.com/USER/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Install the packages:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Prepare the database

Start MariaDB:

``` bash
sudo mariadb
```

Insert the following SQL block:

``` sql
DROP USER IF EXISTS 'meshcore'@'localhost';
CREATE DATABASE IF NOT EXISTS meshcore
CHARACTER SET utf8mb4
COLLATE utf8mb4_general_ci;
CREATE USER 'meshcore'@'localhost';
GRANT ALL PRIVILEGES ON meshcore.* TO 'meshcore'@'localhost';
FLUSH PRIVILEGES;
```

------------------------------------------------------------------------

# Build the software

Compile everything:

``` bash
make
```

Install the HTML files:

``` bash
sudo cp -R html/* /var/www/html
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

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
