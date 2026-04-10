<p align="right">
  <a href="README.de.md">🇩🇪 Deutsch</a> |
  <a href="README.md">🇬🇧 English</a> |
  <a href="README.es.md">🇪🇸 Español</a> |
  <a href="README.fr.md">🇫🇷 Français</a> |
  <a href="README.it.md">🇮🇹 Italiano</a>
</p>

# MeshCore Web Dashboard

<sub>by DJ0ABR (c) 2026</sub>

Tableau de bord web pour les **nœuds MeshCore**, conçu en priorité pour les systèmes de bureau.

-   fonctionne sous **Linux** (Raspberry Pi, PC, etc.)
-   **aucun smartphone nécessaire**
-   optimisé pour les **écrans de bureau**
-   accessible avec **n'importe quel navigateur moderne** sur le réseau local

------------------------------------------------------------------------

# Fonctionnalités

Le tableau de bord propose notamment :

-   affichage des **nœuds et des rooms**
-   **messages de chat**
-   **gestion des canaux**
-   **découverte des répéteurs**
-   **vue cartographique** des nœuds

------------------------------------------------------------------------

## Captures d'écran

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

# Préparer le matériel

1. Ouvrez le **WebFlasher** sur le site web de MeshCore.
2. Flashez le paquet `Companion-USB`.

------------------------------------------------------------------------

# Installation

Cloner le dépôt :

``` bash
git clone https://github.com/USER/meshcore-webdashboard.git
cd meshcore-webdashboard
```

Installer les paquets :

``` bash
sudo ./install.sh
```

------------------------------------------------------------------------

# Préparer la base de données

Démarrer MariaDB :

``` bash
sudo mariadb
```

Insérer le bloc SQL suivant :

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

# Compiler le logiciel

Compiler l'ensemble :

``` bash
make
```

Installer les fichiers HTML :

``` bash
sudo cp -R html/* /var/www/html
```

------------------------------------------------------------------------

# Démarrer le programme

Connectez le matériel via USB.

Identifier le port série :

``` bash
ls /dev/tty*
```

Exemples :

Si le port est **ttyUSB0** :

``` bash
./meshcore_api
```

Si le port est **ttyACM0** :

``` bash
./meshcore_api /dev/ttyACM0
```

------------------------------------------------------------------------

# Ouvrir le tableau de bord

Dans un navigateur sur le réseau domestique :

    http://IP_du_Raspberry_Pi

------------------------------------------------------------------------

# Première configuration

1.  Cliquez sur l’**icône d’engrenage** en haut à droite.
2.  Dans la fenêtre de configuration, saisissez les données suivantes :

-   **Nom** (espaces et caractères spéciaux autorisés)
-   **Longitude**
-   **Latitude**

3.  Cliquez sur **Apply**.

------------------------------------------------------------------------

# Utilisation

Le tableau de bord est maintenant prêt à être utilisé.

Lorsque des stations sont reçues, elles apparaissent dans la **liste des
nœuds sur la gauche**.

⚠️ Remarque :  
Il peut s’écouler **une heure ou plus** avant que les premiers nœuds n’apparaissent.

## Commandes

- **Clic droit sur un nœud :**  
  Une **carte avec la position** du nœud s’ouvre.

- **Clic gauche sur un chat ou une room :**  
  La **fenêtre de chat s’ouvre**.

------------------------------------------------------------------------

# Repeater Discovery

Pour rechercher des répéteurs MeshCore :

1.  Cliquez sur l’**icône de loupe** en haut à droite.
2.  La fenêtre **Repeater Discovery** s’ouvre.
3.  Cliquez sur **START**.

Les répéteurs accessibles sont recherchés.\
L'opération peut être **répétée plusieurs fois**.

------------------------------------------------------------------------

## License

This project is licensed under the MIT License.

This project is not affiliated with the MeshCore project.
