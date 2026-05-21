# Mini-projet IoT sur micro:bit

Ce depot rassemble la partie embarquee du projet :
- le firmware sender et receiver sur micro:bit
- le protocole radio entre l'objet et la passerelle
- les procedures de build, de flash et de debug serie

Le systeme complet est compose de deux depots du workspace :
- `micro-bit` : firmwares micro:bit sender et receiver, protocole radio, build et flash
- `iot-project` : serveur Python UDP/UART et application Android

## 1. Vue d'ensemble
Architecture du projet :

```text
Capteur BME280 + micro:bit sender
            |
            v
      Radio micro:bit
            |
            v
micro:bit receiver + UART USB
            |
            v
   Serveur Python sur le PC
            |
            v
      Application Android
```

Fonctionnalites couvertes :
- lecture de temperature, luminosite, humidite et pression
- affichage local des mesures sur ecran OLED SSD1306 cote sender
- transmission radio securisee sender -> receiver
- ACK radio retour receiver -> sender
- export UART receiver -> PC
- pont UDP/UART cote serveur
- affichage des mesures sur Android
- envoi d'une configuration d'affichage depuis Android vers le sender

## 2. Materiel reel utilise
- 2 cartes micro:bit
- 1 breadboard
- 4 fils
- 1 ecran OLED SSD1306 en I2C
- 1 capteur Techno-Innov compatible BME280
- 1 Mac hote avec conteneur Linux pour le firmware

## 3. Architecture materielle
### 3.1 Repartition des roles
- micro:bit sender : carte objet, reliee au capteur BME280 et a l'ecran OLED
- micro:bit receiver : carte passerelle, reliee en USB au PC pour la liaison UART avec le serveur

### 3.2 Ou brancher l'ecran OLED
L'ecran OLED doit etre branche sur la micro:bit sender, pas sur la receiver.

Le sender est l'objet deployee dans le bureau. C'est lui qui :
- lit les capteurs
- recoit l'ordre d'affichage venant d'Android via la passerelle
- affiche localement les mesures dans l'ordre demande

### 3.3 Cablage OLED SSD1306
Mode minimal valide en 4 fils :
- `3V` micro:bit -> `VCC` OLED
- `GND` micro:bit -> `GND` OLED
- `P20 / SDA` micro:bit -> `SDA` OLED
- `P19 / SCL` micro:bit -> `SCL` OLED

Mode 5 fils optionnel :
- ajouter `P0` micro:bit -> `RST` OLED

Le firmware prend en charge le mode 4 fils sans reset materiel obligatoire.
Le reset sur `P0` reste utile si l'ecran se comporte mal apres certains redemarrages ou reflashes.

### 3.4 Cablage capteur + OLED sur la meme breadboard
Le BME280 et l'OLED partagent le meme bus I2C du sender :
- meme `3V`
- meme `GND`
- meme `SDA` sur `P20`
- meme `SCL` sur `P19`

La breadboard sert donc surtout a distribuer proprement alimentation et lignes I2C.

## 4. Firmware valide
- sender : `source/main.cpp` - version `S6`
- receiver : `source/main2.cpp` - version `R6`
- groupe radio : `83`
- bande radio : `7`
- puissance radio : `7`
- prefixe de protocole : `IOT1|`
- secret partage : `MB26`

Points d'implementation :
- le sender met a jour l'OLED selon `TLH`, `LTH`, `THL` ou `TLHP`
- le driver SSD1306 accepte un reset optionnel
- si le BME280 est absent, le sender bascule sur la temperature interne sans bloquer le boot

## 5. Identifier sender et receiver quand on les branche
Le moyen le plus simple est de regarder le boot affiche sur la matrice LED et sur la console serie.

### 5.1 Sender
Boot attendu :

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 OK
```

Repere physique :
- c'est la carte branchee au capteur et a l'OLED
- la matrice LED affiche `S6` au demarrage

### 5.2 Receiver
Boot attendu :

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

Repere physique :
- c'est la carte reliee au PC pour le serveur Python
- la matrice LED affiche `R6` au demarrage

Conseil pratique :
- brancher une seule carte a la fois au moment du flash
- coller ensuite une etiquette physique `SENDER` et `RECEIVER`

## 6. Demarrage rapide
### 6.1 Build et flash du receiver

```bash
cd /workspaces/micro-bit
make flash-receiver
```

Boot attendu :

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

### 6.2 Build et flash du sender

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

Si le capteur BME280 n'est pas detecte, le boot devient typiquement :

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 ABSENT, fallback temp interne
```

Dans ce cas, le sender continue de fonctionner et l'OLED peut quand meme afficher :
- temperature interne
- luminosite
- humidite et pression a `--`

### 6.3 Lancement du serveur sur le PC hote

Depuis le repo `iot-project` :

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install pyserial
python server/serveur.py --serial-port /dev/cu.usbmodemXXXXX --udp-port 10000
```

### 6.4 Connexion Android
- IP : adresse IP locale du PC qui lance le serveur
- Port : `10000`
- l'application envoie `GET` pour tester la connexion puis `subscribe()` a l'ouverture de l'ecran principal

## 7. Voir la console serie des micro:bit
Les deux firmwares ecrivent leurs traces sur l'USB serie a `115200` bauds.

Parametres serie a utiliser :
- debit : `115200`
- format : `8N1`
- pas de controle de flux

### 7.1 macOS
Lister les ports serie :

```bash
ls /dev/cu.usbmodem*
```

Ouvrir une console sur une micro:bit :

```bash
screen /dev/cu.usbmodemXXXXX 115200
```

Quand deux micro:bit sont branchees :
- ouvrir deux terminaux
- lancer `screen` sur chaque port
- identifier la carte par son boot `S6` ou `R6`

Quitter `screen` proprement :
- `Ctrl-A`, puis `K`, puis confirmer avec `y`

### 7.2 Linux
Lister les ports serie :

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Ouvrir une console avec `screen` :

```bash
screen /dev/ttyACM0 115200
```

Alternative avec `picocom` :

```bash
picocom -b 115200 /dev/ttyACM0
```

Selon la distribution, il peut falloir etre dans le groupe `dialout`.

### 7.3 Windows
Reperer le port COM :
- ouvrir le Gestionnaire de peripheriques
- regarder la section `Ports (COM et LPT)`
- noter le port de la micro:bit, par exemple `COM5`

Alternative PowerShell :

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description
```

Ouvrir une console serie :
- avec PuTTY : connexion `Serial`, port `COM5`, vitesse `115200`
- avec Tera Term : `Serial`, port `COM5`, `115200`

### 7.4 Quelle carte regarder selon le besoin
- console du sender : utile pour verifier capteur, OLED, emissions radio et ACK retour
- console du receiver : utile pour verifier reception radio, export UART et commandes de configuration venant du serveur

## 8. Interpreter les traces micro:bit
### 8.1 Traces typiques du sender

Boot :

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 OK
```

Emission de donnees :

```text
TX: IOT1|D:69|25|98|42|996|CE90B
RX: IOT1|A:69|C1AA3
```

Interpretation :
- `TX:` : le sender vient d'envoyer une trame radio de donnees
- `D:69` : type donnees, sequence `69`
- `25|98|42|996` : temperature, luminosite, humidite, pression
- suffixe `|C...` : tag d'authentification
- `RX: IOT1|A:69|...` : ACK valide recu du receiver pour cette meme sequence

Autres traces sender utiles :
- `ACK non recu` : le receiver n'a pas repondu a temps ou la radio n'est pas bonne
- `CFG appliquee: TLH` : un ordre d'affichage a ete recu et applique sur l'OLED
- `TX CFG-ACK: ...` : le sender confirme la prise en compte de la configuration
- `RX rejetee: auth invalide` : une trame recue ne passe pas la verification d'integrite

### 8.2 Traces typiques du receiver

Boot :

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

Reception radio + export PC :

```text
RX: IOT1|D:69|25|98|42|996|CE90B
TX: IOT1|A:69|C1AA3
DATA T=25C L=98 H=42% P=996hPa
DATA|69|25|98|42|996
```

Interpretation :
- `RX:` : le receiver a recu une trame radio du sender
- `TX:` : le receiver renvoie l'ACK radio au sender
- `DATA T=...` : resume humain utile pour le debug
- `DATA|...` : ligne machine pour le serveur Python

Autres traces receiver utiles :
- `CFG-TX|TLH` : une commande de configuration vient d'etre envoyee au sender
- `CFG-ACK|12|TLH` : le sender a confirme la configuration
- `CFG-ERR|ordre invalide` : la commande recue sur l'UART n'est pas valide
- `DROP: replay` : trame radio ancienne ou dupliquee
- `DROP: auth invalide` : trame radio rejetee pour tag invalide

### 8.3 Traces cote serveur
Le serveur Python lit la console machine du receiver et affiche par exemple :

```text
SERIAL> DATA|69|25|98|42|996
UDP<10.42.233.42:51234> GET
UDP<10.42.233.42:51234> TLH
SERIAL> CFG-ACK|12|TLH
```

Interpretation :
- `SERIAL>` : ligne recue depuis la micro:bit receiver
- `UDP<...>` : commande recue depuis Android ou un client UDP
- `CFG-ACK` : confirmation du changement d'ordre d'affichage sur le sender

## 9. Workflow de verification conseille
### 9.1 Verification sender seul
- brancher le sender
- ouvrir la console serie
- verifier `BOOT: S6`
- verifier `SENSOR: BME280 OK` si le capteur est branche
- verifier que l'OLED affiche `ORDRE: TLHP` puis les mesures

### 9.2 Verification receiver seul
- brancher le receiver
- ouvrir la console serie
- verifier `BOOT: R6`
- verifier `UART commandes: CFG|TLHP`

### 9.3 Verification radio complete
- alimenter les deux cartes
- regarder la console sender : `TX:` puis `RX: ... A:...`
- regarder la console receiver : `RX:` puis `TX:` puis `DATA|...`

### 9.4 Verification serveur + Android
- laisser le receiver branche au PC
- lancer `serveur.py`
- verifier les lignes `SERIAL> DATA|...`
- connecter Android sur l'IP du PC, port `10000`
- verifier qu'un clic sur `TLH`, `LTH` ou `THL` produit ensuite `CFG-ACK|...`

## 10. Points d'attention importants
- le Bluetooth doit rester desactive dans `config.json` pour utiliser la radio bas niveau micro:bit
- ne flasher qu'une seule carte a la fois
- la receiver doit rester branchee en USB au PC pour le serveur
- la sender peut etre alimentee par un autre port USB ou une batterie
- l'OLED est gere cote sender et partage l'I2C avec le BME280
- si l'OLED reste noir, verifier d'abord `3V`, `GND`, `P20`, `P19`, puis l'adresse I2C du module
- le build Android doit etre fait sur la machine hote si le conteneur n'a pas Java/Android SDK

## 11. Documents de reference
- `docs/projet-mini-architecture-iot.md` : vue d'ensemble fonctionnelle et perimetre du projet
- `docs/guide-implementation-iot.md` : procedure complete de montage, flash, serveur et Android
- `docs/protocole-radio-final.md` : specification radio, UART et UDP
- `docs/workflow-android-passerelle.md` : integration Android <-> serveur <-> micro:bit
- `docs/compte-rendu-realisation-radio-capteur-securite.md` : explications techniques et choix d'implementation
- `docs/Rapport_Mini_Projet_IOT.pdf` : rapport synthetique fourni pour le rendu

## 12. Documentation du code sur l'ensemble du workspace
Le code du projet est reparti sur deux depots du workspace.

Documentation a consulter selon la brique :
- `README.md` : vue globale firmware micro:bit
- `../iot-project/README.md` : vue globale serveur + Android
- `../iot-project/server/README.md` : passerelle Python UDP/UART
- `../iot-project/android/README.md` : application Android

## 13. Fichiers principaux de ce repo
- `source/main.cpp` : firmware sender
- `source/main2.cpp` : firmware receiver
- `source/build_role.h` : selection du role au build
- `source/bme280.h` et `source/bme280.cpp` : wrappers du driver capteur
- `source/ssd1306.h` et `source/ssd1306.cpp` : wrappers du driver OLED
- `config.json` : desactivation du Bluetooth
- `Makefile` : build/flash sender et receiver

## 14. Etat du projet
Le projet est fonctionnel de bout en bout :
- le sender envoie des mesures reelles
- le sender affiche localement les mesures sur OLED selon l'ordre configure
- le receiver confirme la reception et exporte les valeurs vers le PC
- le serveur diffuse les donnees vers Android
- Android affiche les mesures et envoie les ordres de configuration

Pistes d'amelioration possibles :
- filtrage des valeurs aberrantes du capteur
- support de plusieurs objets
- secret partage configurable
- autodetection plus fine de l'adresse I2C OLED selon les modules
