# Guide d'implementation final - micro:bit, serveur et Android

## 1. Objet du document
Ce guide explique pas a pas comment reproduire l'installation complete qui a ete validee :
- montage du capteur
- build et flash des deux micro:bit
- lancement du serveur Python
- connexion de l'application Android
- test de bout en bout

Il s'agit du document pratique de reference pour rejouer la demo.

## 2. Prerequis
### 2.1 Repos utilises
- repo firmware : `micro-bit`
- repo serveur + Android : `iot-project`

### 2.2 Outils et environnement
- conteneur Linux pour compiler le firmware micro:bit
- yotta deja installe dans le conteneur
- Mac ou machine hote pour lancer le serveur et Android Studio
- Python 3 sur la machine hote
- `pyserial` installe dans un environnement virtuel Python

### 2.3 Contraintes a respecter
- une seule micro:bit branchee a la fois pendant le flash
- la receiver doit rester branchee au PC pendant l'execution du serveur
- la sender doit rester alimentee en meme temps que la receiver
- Android et le PC doivent etre sur le meme reseau local

## 3. Montage materiel
### 3.1 Repartition des roles
- micro:bit A : sender
- micro:bit B : receiver

### 3.2 Cablage du capteur sur la sender
Montage I2C retenu avec 4 fils :
- `3V` micro:bit -> `VCC` capteur
- `GND` micro:bit -> `GND` capteur
- `P20` micro:bit -> `SDA` capteur
- `P19` micro:bit -> `SCL` capteur

Si le capteur n'est pas reconnu, le firmware bascule en mode fallback :
- temperature = capteur interne micro:bit
- humidite = -1
- pression = -1

## 4. Build et flash du firmware
### 4.1 Build du receiver

```bash
cd /workspaces/micro-bit
make build-receiver
```

### 4.2 Flash du receiver
Brancher uniquement la micro:bit receiver, puis :

```bash
cd /workspaces/micro-bit
make flash-receiver
```

Boot attendu :

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

### 4.3 Build du sender

```bash
cd /workspaces/micro-bit
make build-sender
```

### 4.4 Flash du sender
Debrancher la receiver, brancher uniquement la sender, puis :

```bash
cd /workspaces/micro-bit
make flash-sender
```

Boot attendu :

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 OK
```

### 4.5 Pourquoi une seule carte a la fois
Cette discipline evite trois erreurs frequentes :
- flasher la mauvaise carte
- croire qu'un flash a reussi alors qu'une autre carte etait montee
- confondre les ports serie sender et receiver

## 5. Verification serie locale
### 5.1 Identifier les ports serie
Sur macOS :

```bash
ls /dev/cu.usbmodem*
```

Sur Linux :

```bash
ls /dev/ttyACM*
```

### 5.2 Lire les logs d'une carte

```bash
screen /dev/cu.usbmodemXXXXX 115200
```

Pour quitter `screen` :
- `Ctrl + A`
- `K`
- `y`

### 5.3 Logs attendus en fonctionnement normal
Sender :

```text
TX: IOT1|D:68|25|106|42|996|CXXXX
RX: IOT1|A:68|CXXXX
```

Receiver :

```text
RX: IOT1|D:68|25|106|42|996|CXXXX
TX: IOT1|A:68|CXXXX
DATA T=25C L=106 H=42% P=996hPa
DATA|68|25|106|42|996
```

## 6. Lancement du serveur Python
### 6.1 Pourquoi un environnement virtuel Python
Sur macOS avec Python gere par Homebrew, `pip install` global peut echouer avec l'erreur `externally-managed-environment`.
La solution propre est d'utiliser un venv.

### 6.2 Creation du venv
Depuis le repo `iot-project` sur la machine hote :

```bash
cd /chemin/vers/iot-project
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install pyserial
```

### 6.3 Lancement du serveur
La receiver doit etre la carte branchee au PC.

```bash
python server/serveur.py --serial-port /dev/cu.usbmodemXXXXX --udp-port 10000
```

### 6.4 Ce que le serveur doit afficher
Au demarrage :

```text
Gateway server started
```

Puis, si la radio fonctionne :

```text
SERIAL> RX: IOT1|D:69|25|98|42|996|CE90B
SERIAL> TX: IOT1|A:69|C1AA3
SERIAL> DATA T=25C L=98 H=42% P=996hPa
SERIAL> DATA|69|25|98|42|996
```

Si le serveur ne montre que `Gateway server started`, le probleme est avant lui :
- mauvaise carte branchee
- sender non alimentee
- radio qui ne tourne pas

## 7. Application Android
### 7.1 Emplacement du code
Le code Android final se trouve dans `iot-project/android`.

### 7.2 Build recommande
Le plus simple est d'utiliser Android Studio sur la machine hote.

Alternative ligne de commande si Java et Android SDK sont deja installes :

```bash
cd /chemin/vers/iot-project/android
bash ./gradlew assembleDebug
```

### 7.3 Parametres a saisir dans l'application
- IP : adresse IP locale du PC qui lance le serveur
- Port : `10000`

Cas reel sur smartphone :
- utiliser l'IP LAN du PC, par exemple `10.42.233.151`

Cas emulation Android sur la meme machine :
- `10.0.2.2` peut servir d'adresse par defaut

### 7.4 Comportement normal de l'application
Ecran de connexion :
- envoie `GET` pour verifier que le serveur repond

Ecran principal :
- ouvre une socket UDP persistante
- envoie `subscribe()` et `GET`
- affiche temperature, luminosite, humidite et pression
- permet d'envoyer `TLH`, `LTH` ou `THL`

## 8. Scenario de test complet
### 8.1 Preparation
1. flasher la receiver
2. flasher la sender
3. laisser la receiver branchee au PC
4. alimenter la sender
5. lancer le serveur

### 8.2 Verification micro:bit -> serveur
Le serveur doit afficher des lignes `DATA|...` en continu.

### 8.3 Verification serveur -> Android
1. connecter Android a l'IP du PC et au port `10000`
2. verifier que les mesures s'affichent
3. verifier que le terminal serveur montre des lignes `UDP<...> GET` puis `UDP<...> subscribe()`

### 8.4 Verification Android -> sender
1. appuyer sur `TLH`, `LTH` ou `THL` dans Android
2. verifier que le serveur affiche la commande UDP
3. verifier qu'un `CFG-ACK|...` revient sur l'UART
4. verifier que l'application passe a l'etat `Configuration appliquee`

## 9. Depannage
### 9.1 Sender et receiver affichent seulement leur boot
Verifier :
- que la sender est bien alimentee en continu
- que les deux firmwares flashes sont les bons
- que la sender a bien ete reflashee avec la version finale actuelle

### 9.2 Le sender affiche `ACK non recu`
Verifier :
- la presence du receiver
- la proximite entre les cartes
- le bon groupe radio
- l'absence d'ancien firmware sur une des cartes

### 9.3 Le serveur ne voit pas `DATA|...`
Verifier :
- que le port serie appartient bien au receiver
- que `screen` ou un autre terminal n'utilise pas deja ce port
- que la sender tourne et envoie bien ses trames

### 9.4 Android ne se connecte pas
Verifier :
- la bonne IP du PC
- le port `10000`
- le reseau Wi-Fi commun entre smartphone et PC
- l'absence de firewall bloquant l'UDP

### 9.5 Une valeur capteur aberrante apparait une fois
Une valeur isolee anormale peut encore apparaitre ponctuellement.
Ce n'est pas bloquant pour la demo tant que les trames suivantes redeviennent coherentes.

## 10. Checklist finale avant rendu
- sender en `S6`
- receiver en `R6`
- radio stable avec ACK
- lignes `DATA|...` visibles dans le serveur
- Android connecte au serveur
- affichage des mesures OK
- commande TLH/LTH/THL OK
- procedure de lancement connue par l'equipe
