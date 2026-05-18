# Mini architecture IoT - Dossier de synthese final

## 1. Objet du document
Ce document presente la version finale du mini-projet IoT telle qu'elle a ete reellement implementee et validee.

Il repond a trois objectifs :
- resumer le besoin initial et son adaptation au materiel reel
- decrire l'architecture finale retenue
- servir de porte d'entree vers les documents techniques detailles

## 2. Besoin initial et adaptation au materiel reel
L'enonce demandait une chaine IoT complete comprenant :
- un objet capteur sur micro:bit
- une passerelle reliee au PC
- un serveur
- une application Android
- une commande a distance de l'ordre d'affichage des mesures

Le materiel reel du groupe etait plus simple que l'enonce theorique :
- 2 micro:bit
- 1 breadboard
- 4 fils
- 1 capteur Techno-Innov base sur BME280
- pas d'ecran OLED dedie dans la maquette finale

Adaptation retenue :
- la chaine complete capteur -> radio -> serveur -> Android a ete implemente
- la commande distante d'ordre d'affichage a bien ete implemente cote protocole et cote firmware
- en l'absence d'OLED branche dans la maquette finale, la micro:bit sender memorise l'ordre applique et le confirme par la sortie serie et par un defilement sur la matrice LED

## 3. Architecture finale retenue
### 3.1 Composants
- micro:bit sender : carte objet connectee au capteur
- micro:bit receiver : carte passerelle branchee en USB au PC
- serveur Python : pont UART <-> UDP sur le PC
- application Android : visualisation et configuration

### 3.2 Liaisons
- sender <-> receiver : radio micro:bit 2.4 GHz
- receiver <-> PC : UART sur USB a 115200 bauds
- Android <-> serveur : UDP sur le reseau local

### 3.3 Vue d'ensemble

```text
Capteur BME280
    |
    v
micro:bit sender
    |
    v
radio securisee
    |
    v
micro:bit receiver
    |
    v
UART USB
    |
    v
serveur Python
    |
    v
UDP
    |
    v
application Android
```

## 4. Role de chaque composant
### 4.1 Sender
Le sender :
- lit le capteur BME280
- lit aussi la luminosite native du micro:bit
- construit une trame radio securisee
- envoie cette trame au receiver
- attend un ACK radio
- applique les commandes d'ordre d'affichage recues

### 4.2 Receiver
Le receiver :
- recoit les trames radio du sender
- verifie leur integrite
- rejette les replays simples
- renvoie un ACK radio au sender
- convertit les donnees en lignes UART pour le PC
- relaie les commandes de configuration recues depuis le PC vers le sender

### 4.3 Serveur PC
Le serveur final se trouve dans le repo `iot-project`, fichier `server/serveur.py`.

Il :
- ouvre le port serie du receiver
- lit en continu les lignes `DATA|...` et `CFG-ACK|...`
- conserve la derniere mesure recue
- expose un port UDP pour Android
- accepte les commandes `GET`, `subscribe()`, `getStatus()` et les ordres d'affichage

### 4.4 Application Android
L'application Android se trouve dans le repo `iot-project/android`.

Elle :
- permet de saisir l'IP et le port du serveur
- teste la connexion en UDP
- ouvre ensuite l'ecran principal si le serveur repond
- garde ensuite une socket UDP persistante ouverte
- affiche temperature, luminosite, humidite et pression
- envoie des ordres de configuration d'affichage

## 5. Flux fonctionnels reellement valides
### 5.1 Flux montant : capteur vers Android
1. le sender lit temperature, humidite, pression et luminosite
2. il envoie une trame radio `IOT1|D:...|CXXXX`
3. le receiver valide la trame puis renvoie `IOT1|A:...|CXXXX`
4. le receiver publie `DATA|seq|temp|light|hum|press` sur l'UART
5. le serveur convertit cette ligne en JSON UDP
6. Android affiche les valeurs

### 5.2 Flux descendant : Android vers sender
1. Android envoie une commande UDP `TLH`, `LTH`, `THL` ou `CFG|...`
2. le serveur la traduit en `CFG|ordre` sur l'UART
3. le receiver transforme cette commande en trame radio `IOT1|G:...|ordre|CXXXX`
4. le sender applique la configuration et renvoie `IOT1|K:...|ordre|CXXXX`
5. le receiver emet `CFG-ACK|seq|ordre` vers le PC
6. le serveur pousse un JSON `config_ack` a Android

## 6. Etat final effectivement valide
### 6.1 Ce qui fonctionne
- build separe sender / receiver
- flash separe sender / receiver
- lecture capteur BME280
- transport des mesures T/L/H/P
- ACK radio dans les deux sens utiles
- controle d'integrite des trames
- passerelle UART vers le PC
- serveur UDP fonctionnel
- application Android connectee au serveur
- configuration d'affichage envoyee depuis Android

### 6.2 Ce qui a ete adapte par rapport a l'enonce
- pas d'OLED branche dans la maquette finale
- pas de support multi-objet
- secret partage fixe dans le firmware
- securite legere mais suffisante pour le niveau du projet

## 7. Fichiers importants
Dans `micro-bit` :
- `source/main.cpp` : sender final `S6`
- `source/main2.cpp` : receiver final `R6`
- `config.json` : desactivation du Bluetooth
- `Makefile` : build et flash

Dans `iot-project` :
- `server/serveur.py` : passerelle UDP/UART
- `android/app/src/main/java/fr/cpe/iotudpapp/ConnectionActivity.kt` : ecran de connexion
- `android/app/src/main/java/fr/cpe/iotudpapp/MainActivity.kt` : ecran principal

## 8. Demonstration finale type
Demonstration de reference :
1. flasher le receiver
2. flasher le sender
3. lancer le serveur sur le PC
4. constater les lignes `DATA|...` dans le terminal du serveur
5. connecter Android au serveur
6. verifier l'affichage des mesures
7. envoyer un ordre `TLH` ou `LTH` depuis Android
8. verifier le retour `CFG-ACK|...`

## 9. Limites et evolutions possibles
- filtrage des valeurs aberrantes du capteur
- support de plusieurs objets avec `object_id`
- chiffrement plus fort et secret configurable
- ajout d'un vrai module OLED pour exploiter physiquement l'ordre d'affichage

## 10. Documents complementaires
- `docs/guide-implementation-iot.md`
- `docs/protocole-radio-final.md`
- `docs/workflow-android-passerelle.md`
- `docs/compte-rendu-realisation-radio-capteur-securite.md`
