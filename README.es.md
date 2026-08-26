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
-   y adicionalmente para **dispositivos móviles**
-   acceso con **cualquier navegador moderno** dentro de la red local

------------------------------------------------------------------------

# Funciones

Entre otras cosas, el panel ofrece:

-   visualización de **nodos y salas**
-   **mensajes de chat**
-   **cualquier número de nodos (¡sin límite!)**
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
git clone https://github.com/dj0abr/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Instalar los software:

``` bash
sudo ./install.sh
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

## Consulta del repetidor principal

Además de Repeater Discovery, se puede consultar directamente un **repetidor principal (Home Repeater)**.

- introduce el nombre del Home Repeater y, opcionalmente, la contraseña de administrador
- el inicio de sesión y cada página de la consulta se reintentan automáticamente si falla la transmisión por radio
- la lista completa de vecinos se carga página por página, no solo el primer subconjunto
- los repetidores se comparan con `repeaternodes` mediante los primeros **8 caracteres de la clave pública**
- se añaden las coordenadas disponibles y se calcula la distancia al Home Repeater
- las posiciones claramente no plausibles a más de **400 km** se ignoran para la visualización
- la tabla de resultados se puede ordenar por `#`, pubkey, nombre, "oído hace", SNR y distancia, y se puede copiar al portapapeles
- los repetidores con coordenadas válidas también se pueden mostrar en un **mapa OpenStreetMap** con líneas de conexión al Home Repeater

La contraseña se utiliza únicamente para la consulta actual y no se almacena de forma permanente como contraseña del repetidor.

------------------------------------------------------------------------

## Conexión TCP (MeshCore Companion)

Además de la conexión USB clásica, el backend también puede comunicarse con un MeshCore Companion a través de TCP.

### Requisito

El Companion debe ser flasheado con una imagen especial:
https://www.weyhmueller.org/webtools/esp32_ssid_patcher.html

### Flashear el Companion

1. Conectar el Companion  
2. Abrir la página web  
3. Seleccionar y flashear el firmware  
4. Reiniciar el dispositivo  

### Obtener la dirección IP

Después de un reinicio, la dirección IP se muestra en la pantalla del dispositivo.  
Si el Companion no tiene pantalla, puedes encontrar la IP en tu router (lista DHCP).

### Conexión

```bash
./meshcore_api tcp://<IP>:5000
```

Example:

```bash
./meshcore_api tcp://192.168.10.170:5000
```

------------------------------------------------------------------------

## Lista negra

Se puede colocar un archivo de texto UTF-8 llamado `blacklist.txt` en el directorio del backend.

Si el nombre de un nodo coincide con una entrada de este archivo, el nodo será ignorado y no se escribirá en la base de datos.

- un nombre de nodo por línea
- compatible con UTF-8 / emoji
- las líneas que comienzan con `#` se ignoran
- los repeater nodes en la blacklist se eliminan de `repeaternodes` al iniciar el backend

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
