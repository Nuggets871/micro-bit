# Protocole radio final et procedure de test

## 1. But du document
Ce document formalise le protocole radio final valide dans le projet ainsi que la procedure de build, flash et test.

Il sert de reference rapide pour:
- la demo
- la reprise du projet
- la documentation finale

## 2. Roles des cartes
### 2.1 Sender
Le sender est la carte qui:
- lit les capteurs
- construit une trame de donnees
- envoie la trame par radio
- attend un ACK

Fichier principal:
- source/main.cpp

Version finale:
- S6

### 2.2 Receiver
Le receiver est la carte qui:
- recoit les trames radio
- verifie leur integrite
- affiche les donnees sur UART
- renvoie un ACK

Fichier principal:
- source/main2.cpp

Version finale:
- R6

## 3. Parametres radio
- Groupe radio: 83
- Bande radio: 7
- Puissance radio: 7
- Prefixe de protocole: IOT1|
- Secret partage: MB26

## 4. Capteurs transportes
Le sender transmet les donnees suivantes:
- Temperature en degres Celsius
- Luminosite du micro:bit
- Humidite en pourcentage
- Pression en hPa

Source des donnees:
- temperature, humidite, pression: capteur BME280 externe
- luminosite: micro:bit

Fallback si le BME280 n'est pas detecte:
- temperature interne du micro:bit
- humidite = -1
- pression = -1

## 5. Format des messages
### 5.1 Corps d'une trame de donnees

```text
IOT1|D:<sequence>|<temperature>|<lumiere>|<humidite>|<pression>
```

Exemple:

```text
IOT1|D:18|22|0|51|987
```

### 5.2 Corps d'une trame d'ACK data

```text
IOT1|A:<sequence>
```

Exemple:

```text
IOT1|A:18
```

### 5.3 Corps d'une trame de configuration

```text
IOT1|G:<sequence>|<ordre_affichage>
```

Exemple:

```text
IOT1|G:12|TLHP
```

### 5.4 Corps d'un ACK de configuration

```text
IOT1|K:<sequence>|<ordre_affichage>
```

Exemple:

```text
IOT1|K:12|TLHP
```

### 5.5 Trame finale authentifiee
Chaque corps de message est complete par un tag hexadecimal de 16 bits:

```text
<corps>|CXXXX
```

Exemples:

```text
IOT1|D:18|22|0|51|987|C6CEE
IOT1|A:18|C5574
```

## 6. Regles de validation
### 6.1 Cote receiver
Le receiver ne traite une trame que si:
- elle commence par IOT1|
- elle contient un suffixe |CXXXX
- le tag recalcule correspond au tag recu
- la sequence n'est pas un replay

Si la trame est invalide:
- le receiver affiche DROP: auth invalide

Si la sequence est deja vue:
- le receiver affiche DROP: replay

### 6.2 Cote sender
Le sender considere une trame d'ACK valide seulement si:
- le prefixe est correct
- le tag est valide
- le corps correspond exactement a IOT1|A:<sequence>

## 7. Firmware et messages de boot
### 7.1 Sender
Message de boot attendu:

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
```

Message capteur attendu:

```text
SENSOR: BME280 OK
```

ou en fallback:

```text
SENSOR: BME280 ABSENT, fallback temp interne
```

### 7.2 Receiver
Message de boot attendu:

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
```

## 8. Procedure de build
Depuis la racine du projet:

```bash
cd /workspaces/micro-bit
make build-sender
make build-receiver
```

## 9. Procedure de flash sur macOS
Important:
- ne brancher qu'une seule carte micro:bit a la fois
- verifier qu'un seul volume MICROBIT est monte
- ne jamais flasher les deux cartes en meme temps

### 9.1 Flasher le receiver

```bash
cd /workspaces/micro-bit
make flash-receiver
```

### 9.2 Flasher le sender

```bash
cd /workspaces/micro-bit
make flash-sender
```

## 10. Procedure de test serie
Sur le Mac:

```bash
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodemXXXXX 115200
```

Pour quitter screen:
- Ctrl + A
- K
- y

## 11. Resultat attendu en fonctionnement normal
### 11.1 Sender

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
SENSOR: BME280 OK
TX: IOT1|D:18|22|0|51|987|C6CEE
RX: IOT1|A:18|C5574
```

### 11.2 Receiver

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
RX: IOT1|D:18|22|0|51|987|C6CEE
TX: IOT1|A:18|C5574
DATA T=22C L=0 H=51% P=987hPa
```

## 12. Signification des champs
- sequence: numero de trame
- temperature: temperature en degres Celsius
- lumiere: niveau de lumiere du micro:bit
- humidite: humidite relative en pourcentage
- pression: pression en hPa
- CXXXX: tag d'integrite/authentification

## 13. Lignes UART machine vers le PC
Le receiver emet aussi des lignes machine lisibles par le serveur PC:

```text
DATA|18|22|0|51|987
CFG-ACK|12|TLHP
```

Le PC peut envoyer au receiver des commandes UART comme:

```text
CFG|TLHP
```

## 14. Problemes connus et solutions
### 13.1 Finder se bloque apres flash
Solution pratique:
- debrancher/rebrancher la carte
- flasher une seule carte a la fois

### 13.2 Receiver silencieux apres boot
C'est normal tant qu'aucune trame valide n'est recue.

### 13.3 Messages parasites ou trames vides
Le projet desactive le Bluetooth via config.json, ce qui est necessaire avec le radio datagram micro:bit.

### 13.4 Une carte semble encore en ancienne version
Verifier les messages de boot:
- S6 pour le sender
- R6 pour le receiver

## 15. Limites actuelles
- pas d'identifiant objet
- pas de chiffrement fort
- secret partage en dur
- anti-replay simple
- repo Android non accessible depuis ce conteneur, donc pas encore audite ici

## 16. Prochaine etape recommandee
La suite naturelle du projet est:
- verifier l'application Android sur le protocole UDP du serveur
- stocker les trames cote serveur
- ajouter l'identifiant d'objet pour supporter plusieurs noeuds