<p align="right">
  <a href="README.de.md">🇩🇪 Deutsch</a> |
  <a href="README.md">🇬🇧 English</a> |
  <a href="README.es.md">🇪🇸 Español</a> |
  <a href="README.fr.md">🇫🇷 Français</a> |
  <a href="README.it.md">🇮🇹 Italiano</a>
</p>

# MeshCore Web Dashboard

<sub>by DJ0ABR (c) 2026</sub>

Panel web para **nodos MeshCore** con enfoque en sistemas de escritorio.

-   funciona en **Linux** (Raspberry Pi, PC, etc.)
-   **no se necesita smartphone**
-   optimizado para **monitores de escritorio**
-   acceso con **cualquier navegador moderno** dentro de la red local

------------------------------------------------------------------------

# Funciones

Entre otras cosas, el panel ofrece:

-   visualización de **nodos y salas**
-   **mensajes de chat**
-   **gestión de canales**
-   **descubrimiento de repetidores**
-   **vista de mapa** de los nodos

------------------------------------------------------------------------

## Capturas de pantalla

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

# Preparar el hardware

1. Abre el **WebFlasher** en el sitio web de MeshCore.
2. Flashea el paquete `Companion-USB`.

------------------------------------------------------------------------

# Instalación

Clonar el repositorio:

``` bash
git clone https://github.com/USER/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Instalar los paquetes:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Preparar la base de datos

Iniciar MariaDB:

``` bash
sudo mariadb
```

Insertar el siguiente bloque SQL:

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

# Compilar el software

Compilar todo:

``` bash
make
```

Instalar los archivos HTML:

``` bash
sudo cp -R html/* /var/www/html
```

------------------------------------------------------------------------

# Iniciar el programa

Conecta el hardware por USB.

Determinar el puerto serie:

``` bash
ls /dev/tty*
```

Ejemplos:

Si el puerto es **ttyUSB0**:

``` bash
./meshcore_api
```

Si el puerto es **ttyACM0**:

``` bash
./meshcore_api /dev/ttyACM0
```

------------------------------------------------------------------------

# Abrir el panel

En el navegador dentro de la red doméstica:

    http://IP_de_la_Raspberry_Pi

------------------------------------------------------------------------

# Configuración inicial

1.  Haz clic en el **icono de engranaje** arriba a la derecha.
2.  En la ventana de configuración, introduce los siguientes datos:

-   **Nombre** (se permiten espacios y caracteres especiales)
-   **Longitud**
-   **Latitud**

3.  Haz clic en **Apply**.

------------------------------------------------------------------------

# Funcionamiento

El panel ya está listo para usarse.

Cuando se reciben estaciones, aparecen en la **lista de nodos de la
parte izquierda**.

⚠️ Nota:  
Puede pasar **una hora o más** hasta que aparezcan los primeros nodos.

## Uso

- **Clic derecho sobre un nodo:**  
  Se abre un **mapa con la posición** del nodo.

- **Clic izquierdo sobre un chat o sala:**  
  Se abre la **ventana de chat**.

------------------------------------------------------------------------

# Repeater Discovery

Para buscar repetidores MeshCore:

1.  Haz clic en el **icono de lupa** arriba a la derecha.
2.  Se abre la ventana **Repeater Discovery**.
3.  Haz clic en **START**.

Se buscarán repetidores accesibles.\
El proceso puede **ejecutarse varias veces**.

------------------------------------------------------------------------

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
