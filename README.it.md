<p align="right">
  <a href="README.de.md">🇩🇪 Deutsch</a> |
  <a href="README.md">🇬🇧 English</a> |
  <a href="README.es.md">🇪🇸 Español</a> |
  <a href="README.fr.md">🇫🇷 Français</a> |
  <a href="README.it.md">🇮🇹 Italiano</a>
</p>

# MeshCore Web Dashboard

<sub>by DJ0ABR (c) 2026</sub>

Dashboard web per **nodi MeshCore** con particolare attenzione ai sistemi desktop.

-   funziona su **Linux** (Raspberry Pi, PC, ecc.)
-   **nessuno smartphone richiesto**
-   ottimizzato per **monitor desktop**
-   e anche per **dispositivi mobili**
-   accessibile con **qualsiasi browser moderno** nella rete locale

------------------------------------------------------------------------

# Funzionalità

Il dashboard offre, tra le altre cose:

-   visualizzazione di **nodi e room**
-   **messaggi di chat**
-   **qualsiasi numero di nodi (nessun limite!)**
-   **gestione dei canali**
-   **ricerca dei repeater**
-   **vista mappa** dei nodi

------------------------------------------------------------------------

## Screenshot

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

# Preparare l'hardware

1. Apri il **WebFlasher** sul sito web di MeshCore.
2. Esegui il flash del pacchetto `Companion-USB`.

------------------------------------------------------------------------

# Installazione

Clonare il repository:

``` bash
git clone https://github.com/dj0abr/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Installare i software:

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Avviare il programma

Collega l'hardware tramite USB.

Individua la porta seriale:

``` bash
ls /dev/tty*
```

Esempi:

Se la porta è **ttyUSB0**:

``` bash
./meshcore_api
```

Se la porta è **ttyACM0**:

``` bash
./meshcore_api /dev/ttyACM0
```

------------------------------------------------------------------------

# Aprire il dashboard

Nel browser all'interno della rete domestica:

    http://IP_del_Raspberry_Pi

------------------------------------------------------------------------

# Prima configurazione

1.  Fai clic sull'**icona a forma di ingranaggio** in alto a destra.
2.  Nella finestra di configurazione, inserisci i seguenti dati:

-   **Nome** (spazi e caratteri speciali consentiti)
-   **Longitudine**
-   **Latitudine**

3.  Fai clic su **Apply**.

------------------------------------------------------------------------

# Funzionamento

Il dashboard è ora pronto all'uso.

Quando vengono ricevute stazioni, queste compaiono nella **lista dei nodi
sul lato sinistro**.

⚠️ Nota:  
Potrebbe volerci **un'ora o più** prima che compaiano i primi nodi.

## Uso

- **Clic destro su un nodo:**  
  Si apre una **mappa con la posizione** del nodo.

- **Clic sinistro su una chat o room:**  
  Si apre la **finestra della chat**.

------------------------------------------------------------------------

# Repeater Discovery

Per cercare repeater MeshCore:

1.  Fai clic sull'**icona della lente** in alto a destra.
2.  Si apre la finestra **Repeater Discovery**.
3.  Fai clic su **START**.

Verranno cercati i repeater raggiungibili.\
Il processo può essere **eseguito più volte**.

------------------------------------------------------------------------

## Connessione TCP (MeshCore Companion)

Oltre alla classica connessione USB, il backend può comunicare con un MeshCore Companion anche tramite TCP.

### Requisito

Il Companion deve essere flashato con un’immagine speciale:
https://www.weyhmueller.org/webtools/esp32_ssid_patcher.html

### Flash del Companion

1. Collegare il Companion  
2. Aprire il sito web  
3. Selezionare e flashare il firmware  
4. Riavviare il dispositivo  

### Ottenere l’indirizzo IP

Dopo un riavvio, l’indirizzo IP viene mostrato sul display del dispositivo.  
Se il Companion non ha un display, è possibile trovarlo nel router (lista DHCP).

### Connessione

```bash
./meshcore_api tcp://<IP>:5000
```

Beispiel:

```bash
./meshcore_api tcp://192.168.10.170:5000
```

------------------------------------------------------------------------

## Blacklist

Nel percorso del backend può essere inserito un file di testo UTF-8 chiamato `blacklist.txt`.

Se il nome di un nodo corrisponde a una voce presente in questo file, il nodo verrà ignorato e non sarà scritto nel database.

- un nome nodo per riga
- supporto UTF-8 / emoji
- le righe che iniziano con `#` vengono ignorate
- i repeaternodes nella blacklist vengono rimossi da `repeaternodes` all'avvio del backend

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
