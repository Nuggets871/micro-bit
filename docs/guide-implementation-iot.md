# Guide d'implementation - Mini architecture IoT (micro:bit)

## 1. But du document
Ce document explique comment realiser concrètement le TP IoT avec le code du projet, de la carte micro:bit jusqu'au serveur et a l'application Android.

Il complete le document de cadrage:
- docs/projet-mini-architecture-iot.md

Documents de reference sur l'etat final valide:
- docs/compte-rendu-realisation-radio-capteur-securite.md
- docs/protocole-radio-final.md

## 1.1 Materiel disponible (votre groupe)
- 2 cartes micro:bit
- 1 breadboard
- 4 cables
- 1 capteur TemperatureTechno Inov

Consequences pratiques:
- Une micro:bit peut etre dediee a l'objet capteur
- Une micro:bit peut etre dediee a la passerelle USB vers PC
- Le cablage doit rester minimal et stable (seulement 4 cables)

## 2. Architecture retenue
### 2.1 Composants
- Objet: micro:bit + capteur TemperatureTechno Inov (+ OLED si disponible)
- Passerelle radio: seconde micro:bit connectee en USB au PC
- Serveur: script Python sur PC
- Client: application Android

### 2.2 Liaisons
- Objet <-> Passerelle: radio 2.4 GHz (groupe radio commun)
- Passerelle <-> Serveur: UART sur USB (115200 bauds)
- Android <-> Serveur: UDP (port 10000 par defaut)

### 2.3 Repartition recommandee des 2 micro:bit
- Micro:bit A (objet): lit le capteur TemperatureTechno Inov et envoie les mesures en radio
- Micro:bit B (passerelle): recoit la radio et transfere sur UART vers le serveur, puis renvoie les configurations vers A

## 3. Etat actuel du code dans ce repo
L'etat final valide du repo n'est plus un simple bridge radio/serie.

Le projet implemente maintenant deux firmwares distincts:
- source/main.cpp : sender final S5
- source/main2.cpp : receiver final R5

Fonctionnement actuel:
- radio bidirectionnelle sur le groupe 83
- prefixe de protocole IOT1|
- lecture du capteur BME280 cote sender
- envoi des donnees T, L, H, P
- ACK retour cote receiver
- verification d'un tag d'integrite sur chaque trame

Voir pour le detail complet:
- docs/compte-rendu-realisation-radio-capteur-securite.md
- docs/protocole-radio-final.md

## 4. Build et flash
### 4.1 Compiler
Depuis la racine du projet:
```bash
make build-sender
make build-receiver
```

### 4.2 Flasher la carte
```bash
make flash-receiver
make flash-sender
```

Le workflow recommande sur macOS est:
- brancher une seule carte micro:bit a la fois
- flasher le receiver
- debrancher la carte
- brancher le sender
- flasher le sender

Le Makefile gere une copie locale vers le volume MICROBIT expose dans le conteneur.

### 4.3 Verification serie (Linux/macOS)
Verifier le port:
```bash
ls /dev/cu.usbmodem* /dev/ttyACM* 2>/dev/null
```

Exemple de connexion terminal:
```bash
screen /dev/cu.usbmodemXXXXX 115200
```

Sortie de screen:
- Ctrl + A puis Ctrl + K puis confirmer avec y

## 5. Protocole de messages conseille
## 5.1 Version 1 (simple texte)
Format recommande:
```text
TYPE;OBJECT_ID;TIMESTAMP;PAYLOAD
```

Exemples:
```text
DATA;OBJ01;1710000000;T=23.4;L=460;H=48
CFG;OBJ01;1710000012;ORDER=TLH
```

## 5.2 Version 2 (JSON)
Option d'evolution:
```json
{
  "type": "DATA",
  "object_id": "OBJ01",
  "timestamp": 1710000000,
  "values": {"T": 23.4, "L": 460, "H": 48, "P": 1012}
}
```

## 6. Plan d'implementation par brique
## 6.1 Objet (micro:bit capteur + OLED)
- Lire periodiquement les capteurs
- Encoder les donnees (brut puis JSON)
- Envoyer par radio a la passerelle
- Ecouter les messages CFG
- Mettre a jour l'ordre d'affichage OLED

Contraintes de cablage (4 cables):
- Prioriser VCC, GND et une liaison de donnees stable selon l'interface du capteur
- Eviter les montages complexes sur breadboard pour limiter les faux contacts

## 6.2 Passerelle micro:bit
- Garder le bridge radio/serie deja implemente
- Ajouter un filtrage par object_id si necessaire
- Ajouter une validation minimale de trame

## 6.3 Serveur
- Lire UART en continu
- Sauvegarder les donnees brutes
- Ecouter UDP sur 10000
- Repondre a getValues()
- Relayer les ordres d'affichage recus depuis Android vers UART

Reference de depart:
- https://github.com/CPELyon/4irc-aiot-mini-projet/blob/master/controller.py

## 6.4 Application Android
- Formulaire IP + port
- Ecran ordre d'affichage (T, L, H, P...)
- Envoi UDP de la configuration
- Reception UDP des donnees depuis le serveur
- Filtrage source (IP/port serveur configure)

## 7. Scenarios de test
### 7.1 Test passerelle seule
- Envoyer une ligne sur UART
- Verifier emission radio cote objet de test
- Envoyer une trame radio
- Verifier apparition sur UART

### 7.2 Test bout en bout sans Android
- Objet envoie DATA
- Serveur recoit et stocke
- Injection manuelle d'une CFG vers UART
- Objet met a jour OLED

### 7.3 Test complet
- Android envoie un nouvel ordre (ex: LTH)
- Serveur transmet vers passerelle
- Objet applique l'ordre
- Android recoit les DATA renvoyees par serveur

## 8. Gestion multi-objets
Pour supporter plusieurs objets:
- Rendre object_id obligatoire
- Stockage par object_id cote serveur
- Routage CFG cible par object_id
- Interface Android avec selection d'objet

## 9. Robustesse minimale
- Timeout de lecture UART/radio
- Rejet des trames invalides
- Journal de traces serveur
- Controle d'integrite (checksum/CRC)

## 10. Livrables a fournir
- Rapport synthese (>= 2 pages)
- Code objet documente
- Code passerelle documente
- Application serveur documentee
- Application Android documentee
- Demonstration 10 minutes

## 11. Checklist avant demo
- Build et flash valides
- UART stable a 115200
- Envoi/reception radio ok
- Commande getValues() ok
- Changement d'ordre OLED depuis Android ok
- Affichage des mesures sur smartphone ok
- Cablage capteur valide et reproductible avec 4 cables
- Identification claire des roles micro:bit A (objet) et micro:bit B (passerelle)
