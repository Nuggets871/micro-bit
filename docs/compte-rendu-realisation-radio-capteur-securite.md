# Compte-rendu technique final - radio, capteur, securisation et integration complete

## 1. Objet du document
Ce document raconte la realisation technique du projet dans l'ordre chronologique, jusqu'a la version finale fonctionnelle.

Il sert a expliquer :
- les choix d'architecture
- les principaux problemes rencontres
- les corrections apportees
- le resultat final obtenu

## 2. Contexte reel du projet
### 2.1 Materiel
- 2 micro:bit
- 1 breadboard
- 4 fils
- 1 capteur Techno-Innov compatible BME280
- 1 Mac hote et un conteneur Linux pour la compilation

### 2.2 Objectif
Construire une mini architecture IoT complete capable de :
- lire des mesures environnementales
- transmettre ces mesures d'une micro:bit sender a une micro:bit receiver
- remonter les valeurs jusqu'au PC et a Android
- renvoyer une configuration d'affichage depuis Android jusqu'au sender

## 3. Premiere etape : separer les roles sender / receiver
Au depart, le repo etait celui de `microbit-samples` et ne proposait pas une separation propre entre deux firmwares differents.

Le premier travail a donc ete :
- creer un sender dans `source/main.cpp`
- creer un receiver dans `source/main2.cpp`
- introduire `source/build_role.h`
- adapter le `Makefile` pour produire des artefacts distincts

Commandes finales obtenues :
- `make build-sender`
- `make build-receiver`
- `make flash-sender`
- `make flash-receiver`

## 4. Deuxieme etape : fiabiliser le flash sur macOS
Le workflow de flash a pose des problemes concrets :
- les deux cartes montaient toutes deux sous le nom `MICROBIT`
- Finder pouvait se bloquer apres plusieurs copies
- il etait facile de croire qu'on venait de flasher une carte alors que l'autre etait en fait montee

La methode fiable qui a ete retenue est restee la meme jusqu'a la fin :
- brancher une seule carte a la fois pendant le flash
- verifier le volume monte
- flasher
- debrancher
- passer a l'autre carte

## 5. Troisieme etape : faire marcher la radio brute
La premiere verification utile a ete une communication radio minimale entre les deux cartes.

Le protocole a ensuite ete stabilise autour de :
- groupe `83`
- bande `7`
- puissance `7`
- prefixe `IOT1|`

Un point bloquant a ete rapidement identifie :
- tant que le Bluetooth n'etait pas desactive, la radio datagram micro:bit etait instable

La solution finale a ete d'ajouter `config.json` pour desactiver le Bluetooth.

## 6. Quatrieme etape : transporter de vraies mesures
Une fois la radio fonctionnelle, le sender a ete etendu pour envoyer des donnees utiles.

Le sender lit :
- temperature
- humidite
- pression
- luminosite

Le driver retenu a finalement ete celui deja present dans `source/examples/bme280`, reintegre via des wrappers locaux `source/bme280.h` et `source/bme280.cpp`.

En cas d'absence du capteur, un fallback reste prevu sur la temperature interne.

## 7. Cinquieme etape : ajouter la securisation simple
Le choix retenu pour la securite a ete volontairement leger, car il devait rester compatible avec le micro:bit classic :
- secret partage fixe `MB26`
- calcul d'un tag 16 bits a partir du corps du message
- suffixe `|CXXXX`
- verification du tag a la reception
- anti-replay simple cote receiver

Ce mecanisme a suffi pour :
- filtrer les trames invalides
- garantir que l'ACK recu par le sender correspond bien a la trame envoyee

## 8. Sixieme etape : integrer la configuration d'affichage
Le sender ne devait pas seulement envoyer des donnees. Il devait aussi accepter une commande descendante.

Le chemin final valide est :
- Android envoie `TLH`, `LTH`, `THL` ou `CFG|...`
- le serveur ecrit `CFG|ordre` sur l'UART
- le receiver traduit cela en `IOT1|G:...|ordre|CXXXX`
- le sender applique l'ordre et renvoie `IOT1|K:...|ordre|CXXXX`
- le receiver emet `CFG-ACK|seq|ordre`
- le serveur pousse un JSON `config_ack` a Android

Dans la maquette finale, l'ordre est memorise par le sender et confirme sur la matrice LED et la sortie serie.

## 9. Septieme etape : relier le receiver au PC
La micro:bit receiver a ensuite ete transformee en vraie passerelle UART :
- sorties humaines `RX: ...`, `TX: ...`, `DATA T=...`
- sorties machine `DATA|...`, `CFG-ACK|...`, `CFG-ERR|...`

Cela a permis au PC de devenir independant du protocole radio interne.

## 10. Huitieme etape : aligner le serveur et Android
Le projet a ensuite franchi la derniere couche : l'integration du repo `iot-project`.

Le serveur final a ete implemente dans `iot-project/server/serveur.py`.

Il :
- lit l'UART du receiver
- conserve la derniere mesure
- accepte `GET`, `subscribe()`, `getStatus()` et les ordres d'affichage
- diffuse les mesures et les ACK de configuration en JSON

L'application Android a ete relue et adaptee pour :
- garder une socket UDP persistante
- s'abonner au serveur au demarrage
- afficher temperature, luminosite, humidite et pression
- envoyer des ordres `TLH`, `LTH`, `THL`

## 11. Dernier bug critique trouve juste avant la validation finale
Le symptome etait trompeur :
- le sender ne montrait que son boot
- aucun `TX:` n'apparaissait ensuite
- le receiver restait muet apres son boot

Le probleme n'etait ni le capteur, ni la radio, ni le serveur.

La vraie cause etait un detail du DAL micro:bit :
- `uBit.radio.datagram.recv()` renvoie un `EmptyPacket` de longueur `1` quand aucune trame n'est disponible
- le sender convertissait ce paquet en `ManagedString`
- ce faux message vide faisait tourner la boucle de lecture radio a vide avant meme le premier envoi

La correction finale a consiste a filtrer explicitement ce sentinel dans le sender avant tout traitement radio.

Apres cette correction :
- le sender a recommence a emettre
- le receiver a recommence a ack-er
- le serveur a vu immediatement les lignes `DATA|...`

## 12. Validation finale observee
### 12.1 Cote sender

```text
TX: IOT1|D:69|25|98|42|996|CE90B
RX: IOT1|A:69|C1AA3
```

### 12.2 Cote receiver

```text
RX: IOT1|D:69|25|98|42|996|CE90B
TX: IOT1|A:69|C1AA3
DATA T=25C L=98 H=42% P=996hPa
DATA|69|25|98|42|996
```

### 12.3 Cote serveur

```text
SERIAL> DATA|69|25|98|42|996
```

### 12.4 Cote Android
Validation attendue et obtenue dans la version finale :
- connexion a l'IP/port du serveur
- affichage des mesures
- envoi d'une configuration de mode
- retour `config_ack`

## 13. Ce qui est considere comme termine
- build et flash separes sender / receiver
- radio sender -> receiver stable
- ACK radio retour stable
- lecture capteur BME280
- export UART du receiver
- serveur UDP/UART final
- application Android connectee au serveur
- configuration descendante Android -> sender

## 14. Lecons apprises
- toujours desactiver le Bluetooth pour la radio datagram micro:bit
- ne jamais flasher les deux cartes a la fois sur macOS
- bien distinguer le port serie du sender et celui du receiver
- garder des logs de boot explicites (`S6`, `R6`)
- ne pas supposer qu'un `recv()` vide renvoie une longueur nulle sans verifier l'implementation du DAL

## 15. Limites residuelles non bloquantes
- pas de support multi-objet
- securite volontairement simple
- pas de chiffrement fort
- secret partage code en dur
- pas de filtrage automatique des outliers capteur
- pas d'OLED physique dans la maquette finale

## 16. Conclusion
Le projet peut etre considere comme finalise fonctionnellement.

La chaine complete suivante a ete demontree :

```text
capteur -> sender -> radio -> receiver -> UART -> serveur -> UDP -> Android
Android -> UDP -> serveur -> UART -> receiver -> radio -> sender
```

Le resultat final est coherent avec le cahier des charges pedagogique, adapte au materiel reellement disponible, et suffisamment robuste pour une demonstration complete.
