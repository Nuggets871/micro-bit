# Compte-rendu technique - Realisation radio, capteur et securisation

## 1. Objet du document
Ce document explique en detail ce qui a ete implemente dans le projet pendant la phase objet radio du mini-projet IoT.

Le but de cette phase etait de valider, dans cet ordre:
- la communication radio entre deux micro:bit
- l'envoi de vraies donnees capteur
- l'ajout d'une couche simple de securisation des trames

Le resultat final est une communication fiable entre deux cartes micro:bit, avec accusés de reception, donnees environnementales et verification d'integrite.

## 2. Materiel reel utilise
- 2 cartes micro:bit
- 1 breadboard
- 4 cables
- 1 capteur meteo Techno-Innov base sur BME280
- 1 Mac hote, avec le projet execute dans un conteneur Linux

## 3. Architecture finale validee
### 3.1 Repartition des roles
- Micro:bit A : objet emetteur, appele sender
- Micro:bit B : objet recepteur/passerelle radio, appele receiver

### 3.2 Role du sender
- lire les mesures capteur
- construire une trame radio
- envoyer la trame
- attendre un ACK valide

### 3.3 Role du receiver
- recevoir les trames radio
- verifier leur integrite
- extraire les donnees
- renvoyer un ACK
- afficher les donnees sur la sortie serie

## 4. Fichiers importants du projet
- source/main.cpp : firmware sender final
- source/main2.cpp : firmware receiver final
- source/bme280.h : wrapper local du driver capteur
- source/bme280.cpp : wrapper local du driver capteur
- source/examples/bme280/bme280.h : driver BME280 fourni dans le repo
- source/examples/bme280/bme280.cpp : implementation du driver BME280 fourni dans le repo
- config.json : desactivation du Bluetooth pour liberer la radio bas niveau
- Makefile : cibles de build et flash pour sender et receiver

## 5. Chronologie de la realisation
### 5.1 Separation des deux firmwares
Au debut, le projet ne gerait pas proprement deux micro:bit differents avec deux roles differents.

Nous avons separe les roles en deux firmwares:
- un sender dans source/main.cpp
- un receiver dans source/main2.cpp

Le fichier source/build_role.h permet de choisir quel fichier expose la fonction main lors du build.

Le Makefile a ensuite ete adapte pour fournir des cibles distinctes:
- make build-sender
- make build-receiver
- make flash-sender
- make flash-receiver

## 5.2 Probleme de flash sur macOS
Le poste de travail etait un Mac, alors que le code tournait dans un conteneur Linux.

Deux difficultes ont ete rencontrees:
- les volumes des cartes micro:bit apparaissaient tous sous le nom MICROBIT
- Finder pouvait se bloquer apres plusieurs copies

La methode de travail qui s'est revelee fiable est la suivante:
- ne brancher qu'une seule carte micro:bit a la fois lors du flash
- verifier qu'un seul volume MICROBIT est monte
- flasher la carte voulue
- debrancher, puis brancher l'autre carte

Ce point est important car un faux succes de flash peut arriver si les deux cartes sont branchees ou si la mauvaise carte est montee.

## 5.3 Premier protocole radio de validation
La premiere etape a consiste a valider la radio seule, sans capteur.

Le sender envoyait:
- IOT1|PING:n

Le receiver repondait:
- IOT1|ACK:n

Cette etape a permis de valider:
- la couche radio bas niveau
- le bon groupe radio
- le bon cablage materiel global
- la stabilite du workflow de flash et de debug serie

## 5.4 Probleme radio critique: Bluetooth actif
Un probleme important a ete rencontre pendant les premiers essais:
- des messages vides ou parasites apparaissaient
- les ACK ne revenaient pas correctement

La cause etait connue dans l'ecosysteme micro:bit:
- l'utilisation du radio datagram bas niveau est incompatible avec le Bluetooth actif

La correction a consiste a ajouter un fichier config.json a la racine avec:

```json
{
  "microbit-dal": {
    "bluetooth": {
      "enabled": 0
    }
  }
}
```

Apres cette correction, la radio est devenue exploitable.

## 5.5 Stabilisation du protocole radio
Pour reduire les collisions et le bruit radio, plusieurs choix ont ete fixes:
- groupe radio: 83
- bande radio: 7
- puissance d'emission: 7
- prefixe de protocole: IOT1|

Le prefixe IOT1| permet de filtrer les trames qui ne font pas partie du protocole du projet.

## 5.6 Correction d'un bug de parsing
Pendant les premiers tests du receiver, la carte recevait bien les trames, mais ne renvoyait pas d'ACK.

La cause etait un bug dans l'utilisation de ManagedString::substring(start, length):
- la fonction attend une longueur
- le code lui donnait un comportement equivalent a un index de fin

Apres correction, le receiver a recommence a generer correctement les ACK.

## 5.7 Passage de PING/ACK a DATA/ACK
Une fois la radio validee, le protocole a ete etendu pour transporter de vraies donnees.

La premiere version data utilisait:
- temperature interne du micro:bit
- niveau de lumiere du micro:bit

Les trames resemblaient a:

```text
IOT1|D:98|T:29|L:0
```

Puis le protocole a ete simplifie vers une forme plus compacte pour tenir proprement dans les paquets radio:

```text
IOT1|D:98|29|0|51|987
```

Cette version compacte a servi de base a la couche de securisation finale.

## 5.8 Integration du capteur externe BME280
Une fois la radio stable, le sender a ete relie au capteur externe Techno-Innov base sur BME280.

Le projet contenait deja un exemple BME280 dans:
- source/examples/bme280/bme280.h
- source/examples/bme280/bme280.cpp

Plutot que de conserver un driver maison fragile, le projet a reutilise ce driver existant a travers deux wrappers locaux:
- source/bme280.h
- source/bme280.cpp

Le sender lit alors:
- temperature
- humidite
- pression
- luminosite du micro:bit

En cas d'absence du capteur, un fallback est prevu:
- temperature interne du micro:bit
- humidite = -1
- pression = -1

## 5.9 Ajout d'une couche simple de securite
La derniere etape de cette phase a ete la securisation des trames radio.

Le choix retenu est volontairement simple et compatible avec le niveau du projet:
- secret partage entre sender et receiver
- calcul d'un tag d'integrite a partir du contenu du message
- verification du tag a la reception
- rejet des trames invalides
- protection anti-replay simple cote receiver

Le secret partage choisi est:
- MB26

Le tag est calcule avec une variante simple basee sur FNV-1a 16 bits.

Chaque trame finale contient:
- un corps de message
- un suffixe |CXXXX avec XXXX en hexadecimal

Exemple:

```text
IOT1|D:15|22|0|51|987|C8961
```

Le receiver verifie le tag avant de traiter la trame.
Le sender verifie aussi que l'ACK recu est bien authentifie.

## 6. Protocole final valide
### 6.1 Parametres radio
- groupe: 83
- bande: 7
- puissance: 7

### 6.2 Prefixe de protocole
- IOT1|

### 6.3 Trame de donnees
Format logique du corps:

```text
IOT1|D:<sequence>|<temperature>|<lumiere>|<humidite>|<pression>
```

Puis ajout du tag final:

```text
IOT1|D:<sequence>|<temperature>|<lumiere>|<humidite>|<pression>|CXXXX
```

### 6.4 Trame d'ACK
Format logique du corps:

```text
IOT1|A:<sequence>
```

Puis ajout du tag final:

```text
IOT1|A:<sequence>|CXXXX
```

## 7. Resultat final obtenu
Le systeme final valide les points suivants:
- le sender lit le capteur externe
- le sender envoie les donnees par radio
- le receiver recoit les donnees
- le receiver verifie le tag d'integrite
- le receiver extrait temperature, lumiere, humidite et pression
- le receiver renvoie un ACK authentifie
- le sender valide l'ACK recu

Exemple de logs valides cote receiver:

```text
RX: IOT1|D:14|22|0|51|987|C8CA0
TX: IOT1|A:14|C99C4
DATA T=22C L=0 H=51% P=987hPa
```

Exemple de logs valides cote sender:

```text
TX: IOT1|D:14|22|0|51|987|C8CA0
RX: IOT1|A:14|C99C4
```

## 8. Bilan technique
### 8.1 Ce qui est valide
- communication radio bidirectionnelle
- build et flash distincts pour les deux cartes
- transport de donnees capteur
- ACK fiables
- desactivation du Bluetooth pour liberer la radio
- lecture du capteur BME280
- verification d'integrite des trames

### 8.2 Limites actuelles
- pas encore d'identifiant objet pour gerer plusieurs noeuds
- pas encore de chiffrement fort
- secret partage en dur dans le firmware
- anti-replay volontairement simple
- pas encore de pont complet receiver -> UART -> serveur dans l'etat final S5/R5

## 9. Conclusion
Cette phase du projet peut etre consideree comme reussie.

Le resultat final est une mini-architecture radio micro:bit capable de:
- collecter des mesures reelles
- transporter ces mesures entre deux cartes
- confirmer leur reception
- appliquer une securisation simple mais fonctionnelle

Cette base est suffisante pour passer a l'etape suivante du mini-projet:
- integration passerelle UART/serveur
- support multi-objets
- application Android et configuration distante