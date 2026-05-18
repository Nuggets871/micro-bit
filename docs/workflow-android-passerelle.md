# Workflow complet Android, serveur, micro:bit et capteur

## 1. But du document
Ce document explique le workflow complet attendu entre:
- le capteur et la micro:bit sender
- la micro:bit receiver branchee au PC
- le serveur sur le PC
- l'application Android

L'objectif est de clarifier qui parle a qui, sous quel format et pour quel usage.

## 2. Vue globale du systeme
### 2.1 Micro:bit sender
La carte sender est l'objet deployee.

Elle:
- lit le capteur BME280
- lit la luminosite du micro:bit
- construit une trame radio securisee
- envoie cette trame au receiver
- peut recevoir une configuration d'affichage venant du receiver

### 2.2 Micro:bit receiver
La carte receiver joue le role de passerelle radio.

Elle:
- recoit les trames radio du sender
- verifie leur integrite
- renvoie un ACK radio au sender
- convertit les donnees en lignes UART pour le PC
- recoit des commandes UART depuis le PC
- les transmet par radio au sender

### 2.3 Serveur sur le PC
Le serveur tourne sur le PC relie en USB a la micro:bit receiver.

Il:
- lit les lignes UART du receiver
- stocke la derniere mesure recue
- expose une interface UDP pour l'application Android
- pousse les commandes Android vers la passerelle micro:bit

### 2.4 Application Android
L'application Android ne parle pas directement a la micro:bit.

Elle parle au serveur sur le PC en UDP.

Le chemin reel est donc:

```text
Android <-> UDP <-> Serveur PC <-> UART USB <-> micro:bit receiver <-> Radio <-> micro:bit sender <-> Capteur
```

## 3. Flux montant: capteur vers Android
### 3.1 Lecture capteur
Le sender lit:
- temperature
- humidite
- pression
- lumiere

### 3.2 Trame radio envoyee
Le sender envoie une trame radio securisee de type data:

```text
IOT1|D:<sequence>|<temperature>|<lumiere>|<humidite>|<pression>|CXXXX
```

Exemple:

```text
IOT1|D:18|22|0|51|987|C6CEE
```

### 3.3 Reponse du receiver
Le receiver:
- verifie le tag
- parse les donnees
- renvoie un ACK radio securise

```text
IOT1|A:18|C5574
```

### 3.4 Sortie UART du receiver vers le PC
En plus des logs humains, le receiver emet une ligne machine lisible par le serveur:

```text
DATA|18|22|0|51|987
```

### 3.5 Traitement serveur
Le serveur lit cette ligne UART et met a jour son etat interne.

Ensuite il peut:
- repondre a un `getValues()` Android
- pousser automatiquement la nouvelle mesure a un client abonne

### 3.6 Message UDP vers Android
Le serveur repond en JSON, par exemple:

```json
{
  "type": "data",
  "sequence": 18,
  "temperature": 22,
  "light": 0,
  "humidity": 51,
  "pressure": 987
}
```

## 4. Flux descendant: Android vers micro:bit
### 4.1 But fonctionnel
L'application Android doit pouvoir envoyer une configuration d'affichage au sender.

Dans l'enonce, cela correspond par exemple a des ordres du type:
- TLH
- LTH
- TLPH

## 4.2 Message Android vers serveur
Le serveur accepte:
- une commande brute comme `TLHP`
- ou une commande prefixed comme `CFG|TLHP`

### 4.3 Serveur vers receiver sur UART
Le serveur transforme la demande en commande UART:

```text
CFG|TLHP
```

### 4.4 Receiver vers sender en radio
Le receiver construit une trame radio securisee de configuration:

```text
IOT1|G:<sequence>|TLHP|CXXXX
```

### 4.5 Sender applique la configuration
Le sender:
- verifie le tag
- verifie que l'ordre est valide
- applique la configuration
- renvoie un ACK de configuration

```text
IOT1|K:<sequence>|TLHP|CXXXX
```

### 4.6 Receiver vers serveur
Le receiver convertit cet ACK en ligne UART machine:

```text
CFG-ACK|12|TLHP
```

### 4.7 Serveur vers Android
Le serveur peut renvoyer un JSON du type:

```json
{
  "type": "config_ack",
  "sequence": 12,
  "order": "TLHP"
}
```

## 5. Ce que disent les composants
### 5.1 Sender radio
- `IOT1|D:...|CXXXX` pour les donnees
- `IOT1|K:...|CXXXX` pour ACK de configuration

### 5.2 Receiver radio
- `IOT1|A:...|CXXXX` pour ACK des donnees
- `IOT1|G:...|CXXXX` pour les commandes de configuration

### 5.3 Receiver UART vers PC
- `DATA|seq|temp|light|hum|press`
- `CFG-ACK|seq|order`
- `CFG-ERR|...`

### 5.4 PC vers receiver UART
- `CFG|TLHP`

### 5.5 Android vers serveur UDP
- `getValues()` pour lire la derniere mesure
- `subscribe()` pour recevoir les updates push
- `TLHP` ou `CFG|TLHP` pour envoyer une configuration d'affichage

## 6. Workflow reel a executer
### 6.1 Flasher le receiver

```bash
cd /workspaces/micro-bit
make flash-receiver
```

Attendu au boot:

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

### 6.2 Flasher le sender

```bash
cd /workspaces/micro-bit
make flash-sender
```

Attendu au boot:

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 OK
```

### 6.3 Lancer le serveur PC

```bash
python3 tools/gateway_server.py --serial-port /dev/cu.usbmodemXXXXX --udp-port 10000
```

Note:
- le port serie doit etre celui de la micro:bit receiver branchee au PC
- il faut avoir `pyserial` installe

## 7. Attendus pour l'application Android
L'application Android doit au minimum:
- permettre de renseigner IP et port du serveur
- envoyer des datagrammes UDP au serveur
- parser les JSON recus
- proposer une commande `getValues()`
- proposer une commande de configuration d'affichage

Si l'application sait deja:
- faire du UDP client
- envoyer des strings
- recevoir des reponses JSON

alors elle peut communiquer avec le systeme mis en place ici.

## 8. Limite actuelle importante
Le repo Android indique par l'utilisateur n'est pas accessible dans ce conteneur.

Consequence:
- le contrat serveur <-> Android a ete defini et implemente cote micro:bit + serveur
- mais le code exact de l'application Android n'a pas pu etre audite ni adapte ici

Pour finaliser l'integration Android, il faudra soit:
- monter ce repo Android dans l'environnement actuel
- soit copier ses fichiers utiles dans le workspace

## 9. Resultat final de cette etape
Ce qui est maintenant pret:
- sender capable de lire le capteur et d'envoyer les donnees
- receiver capable de faire la passerelle radio vers UART
- sender capable de recevoir une configuration radio
- receiver capable de recevoir une configuration par UART
- serveur PC capable de faire le pont UART <-> UDP

Il ne manque donc plus, pour valider l'ensemble Android, que l'adaptation ou la verification du code de l'application elle-meme.