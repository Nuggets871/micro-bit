# Mini architecture IoT micro:bit - Version finale

Ce depot ne sert plus de collection generique d'exemples micro:bit.
Il contient le firmware embarque final du mini-projet IoT valide en demonstration.

Le systeme complet est compose de deux depots du workspace :
- `micro-bit` : firmwares micro:bit sender et receiver, protocole radio, build et flash
- `iot-project` : serveur Python UDP/UART et application Android

## 1. Vue d'ensemble
Architecture finale validee :

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

Fonctions validees :
- lecture de temperature, luminosite, humidite et pression
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
- 1 capteur Techno-Innov compatible BME280
- 1 Mac hote avec conteneur Linux pour le firmware

## 3. Firmware valide
- sender : `source/main.cpp` - version `S6`
- receiver : `source/main2.cpp` - version `R6`
- groupe radio : `83`
- bande radio : `7`
- puissance radio : `7`
- prefixe de protocole : `IOT1|`
- secret partage : `MB26`

## 4. Demarrage rapide
### 4.1 Build et flash du receiver

```bash
cd /workspaces/micro-bit
make flash-receiver
```

Boot attendu :

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

### 4.2 Build et flash du sender

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

### 4.3 Lancement du serveur sur le PC hote

Depuis le repo `iot-project` :

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install pyserial
python server/serveur.py --serial-port /dev/cu.usbmodemXXXXX --udp-port 10000
```

### 4.4 Connexion Android
- IP : adresse IP locale du PC qui lance le serveur
- Port : `10000`
- l'application envoie `GET` pour tester la connexion puis `subscribe()` a l'ouverture de l'ecran principal

## 5. Points d'attention importants
- le Bluetooth doit rester desactive dans `config.json` pour utiliser la radio bas niveau micro:bit
- ne flasher qu'une seule carte a la fois
- la receiver doit rester branchee en USB au PC pour le serveur
- la sender peut etre alimentee par un autre port USB ou une batterie
- le build Android doit etre fait sur la machine hote si le conteneur n'a pas Java/Android SDK

## 6. Documents de reference
- `docs/projet-mini-architecture-iot.md` : vue d'ensemble fonctionnelle et perimetre final
- `docs/guide-implementation-iot.md` : procedure complete de montage, flash, serveur et Android
- `docs/protocole-radio-final.md` : specification finale radio, UART et UDP
- `docs/workflow-android-passerelle.md` : integration Android <-> serveur <-> micro:bit
- `docs/compte-rendu-realisation-radio-capteur-securite.md` : historique technique et choix d'implementation

## 7. Fichiers principaux de ce repo
- `source/main.cpp` : firmware sender
- `source/main2.cpp` : firmware receiver
- `source/build_role.h` : selection du role au build
- `source/bme280.h` et `source/bme280.cpp` : wrappers du driver capteur
- `config.json` : desactivation du Bluetooth
- `Makefile` : build/flash sender et receiver

## 8. Etat du projet
Le projet est fonctionnel de bout en bout dans sa version finale :
- le sender envoie des mesures reelles
- le receiver confirme la reception et exporte les valeurs vers le PC
- le serveur diffuse les donnees vers Android
- Android affiche les mesures et envoie les ordres de configuration

Les ameliorations restantes sont du polish, pas des blocages :
- filtrage des valeurs aberrantes du capteur
- support de plusieurs objets
- secret partage configurable
- ajout d'un vrai module OLED si souhaite
