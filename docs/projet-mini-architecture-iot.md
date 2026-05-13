# Developpement embarque et IoT
## Mini-projet 2026 - Mise en place d'une mini-architecture IoT

Document complementaire (implementation pas-a-pas):
- docs/guide-implementation-iot.md

Documentation detaillee de l'implementation validee:
- docs/compte-rendu-realisation-radio-capteur-securite.md
- docs/protocole-radio-final.md

## 1. Contexte du projet
Ce mini-projet consiste a concevoir et implementer une architecture IoT complete autour de trois briques principales :
- Objet connecte (micro:bit + capteurs + ecran OLED)
- Passerelle (micro:bit relie au PC via USB/UART)
- Serveur (application sur PC)
- Client mobile Android (configuration + visualisation)

Le besoin metier est le suivant :
- Relever des donnees environnementales dans des bureaux (temperature, luminosite, humidite, pression, etc.)
- Afficher ces informations sur un ecran OLED cote objet
- Permettre la configuration a distance de l'ordre d'affichage via une application Android
- Stocker et redistribuer les donnees via un serveur

## 2. Objectifs pedagogiques
Le projet permet de mobiliser les competences acquises en TP :
- Concevoir un protocole reseau entre objet et passerelle
- Programmer un microcontroleur pour acquisition, emission et reception de donnees
- Programmer une passerelle/serveur pour reception, traitement, stockage et reemission
- Developper une application Android connectee en UDP pour configurer et afficher des donnees

## 3. Organisation
- Travail en equipe de 4 etudiants
- Inscription du groupe dans le fichier partage du module (Groupe Mini-Projet)

## 3.1 Materiel disponible (groupe)
- 2 micro:bit
- 1 breadboard
- 4 cables
- 1 capteur TemperatureTechno Inov

Impact sur l'architecture:
- Micro:bit 1 dedie a l'objet capteur
- Micro:bit 2 dedie a la passerelle USB/RF
- Cablage compact a privilegier pour fiabiliser la demo

## 4. Architecture cible
### 4.1 Vue globale
1. Objet IoT (micro:bit capteur + OLED) collecte les mesures
2. Objet envoie les donnees en RF 2.4 GHz vers la passerelle
3. Passerelle (seconde micro:bit connectee USB au PC) transmet les donnees au serveur
4. Serveur stocke les mesures et repond aux requetes Android en UDP
5. Application Android envoie des configurations d'affichage (ordre des mesures)
6. Serveur transmet la configuration a la passerelle, puis a l'objet
7. Objet met a jour l'affichage OLED selon la configuration recue

### 4.2 Flux de communication
- Objet <-> Passerelle : RF 2.4 GHz (bidirectionnel)
- Passerelle <-> Serveur : UART USB (bidirectionnel)
- Android <-> Serveur : UDP reseau local (WiFi ou reseau interne emulation)

## 5. Partie 1 - Infrastructure objet-passerelle
### 5.1 Mise en place du reseau RF
Exigences :
- Communication bidirectionnelle objet/passerelle
- Protocole simple, robuste et extensible
- Prise en compte de la securite des donnees
- Possibilite de gerer plusieurs objets deployee dans differents bureaux

Proposition de trame minimale (texte ou binaire) :
- type_message (DATA, CFG, ACK optionnel)
- object_id
- timestamp
- payload
- checksum ou CRC

Exemple logique :
- DATA;OBJ01;1710000000;T=23.4;L=460;H=48
- CFG;OBJ01;1710000012;ORDER=TLH

### 5.2 Configuration des capteurs cote objet
L'objet doit :
- Initialiser les capteurs disponibles (temperature, luminosite, humidite, pression...)
- Echantillonner periodiquement les mesures
- Encoder les mesures dans un message brut (obligatoire au debut)
- Eventuellement evoluer vers JSON (optionnel)

### 5.3 Affichage OLED cote objet
L'objet doit :
- Recevoir une configuration d'affichage depuis la passerelle
- Interpreter l'ordre recu (ex : TLH, LTH, etc.)
- Afficher les mesures sur OLED dans l'ordre demande

Important :
- Ne pas se limiter a T, L, H
- Prevoir l'ensemble des capteurs exposes par votre module meteo

## 6. Partie 2 - Passerelle et serveur
### 6.1 Role de la passerelle (micro:bit sur PC)
- Recevoir les donnees capteurs via RF
- Les transmettre au PC via UART USB
- Recevoir des commandes de configuration depuis le PC
- Reemettre ces commandes vers l'objet cible en RF

### 6.2 Serveur - version de base attendue
Fonctions minimales :
- Lire les donnees recues sur UART
- Stocker les donnees brutes dans un fichier texte
- Ecouter les requetes UDP du client Android sur le port 10000
- Repondre a getValues() avec les donnees disponibles dans le format courant

Reference fournie :
- https://github.com/CPELyon/4irc-aiot-mini-projet/blob/master/controller.py

Points d'attention :
- Verifier le bon port serie associe au micro:bit
- Adapter le script selon OS si necessaire

### 6.3 Evolutions recommandees
Ameliorations possibles (fortement valorisees) :
- Remplacer le stockage texte par une base de donnees (SQLite, MongoDB, InfluxDB)
- Standardiser les echanges (JSON, XML ou autre format defini)
- Ajouter une visualisation web (ex : Grafana)
- Gerer plusieurs objets via un identifiant unique et un routage cible
- Fournir des filtres, historiques, statistiques

## 7. Partie 3 - Application Android
### 7.1 Fonction 1 : definir l'ordre d'affichage
L'app doit permettre de :
- Choisir l'ordre d'affichage des mesures (drag/drop, boutons, liste ordonnee, etc.)
- Convertir ce choix en message de configuration (ex : TLH)
- Envoyer cette configuration au serveur en UDP

### 7.2 Fonction 2 : definir le serveur de destination
L'app doit proposer :
- Champ IP serveur
- Champ port serveur (defaut 10000)
- Envoi UDP sans mecanisme ACK obligatoire

### 7.3 Fonction 3 : communication bidirectionnelle
En plus de l'envoi de configuration, l'app doit aussi :
- Recevoir les mesures envoyees par le serveur
- Afficher les donnees dans l'application
- N'accepter que les messages venant du serveur selectionne (IP/port)

## 8. Protocole de donnees - recommandation pratique
Pour une implementation simple et evolutive :
- Conserver une V1 en donnees brutes pour valider toute la chaine
- Definir ensuite une V2 JSON

Exemple V2 JSON DATA :
```json
{
  "type": "DATA",
  "object_id": "OBJ01",
  "timestamp": 1710000000,
  "values": {
    "T": 23.4,
    "L": 460,
    "H": 48,
    "P": 1012
  }
}
```

Exemple V2 JSON CFG :
```json
{
  "type": "CFG",
  "object_id": "OBJ01",
  "order": ["T", "L", "H", "P"]
}
```

## 9. Strategie de tests (conseillee)
### 9.1 Tests unitaires/techniques
- Validation du parsing des messages
- Validation de l'encodage/decodage des trames
- Verification du mapping capteurs -> symboles d'affichage

### 9.2 Tests d'integration
- Objet -> passerelle RF
- Passerelle -> serveur UART
- Android -> serveur UDP
- Serveur -> passerelle -> objet pour la configuration

### 9.3 Tests fonctionnels
- Changement d'ordre d'affichage depuis Android
- Mise a jour effective OLED dans le bon ordre
- Recuperation et affichage des valeurs sur smartphone

## 10. Securite et robustesse (minimum attendu)
- Ajouter un identifiant unique objet
- Ajouter controle d'integrite (checksum/CRC)
- Ignorer les messages mal formes
- Gerer les timeouts/retransmissions si necessaire
- Journaliser les erreurs cote serveur

## 11. Planning recommande (exemple)
### Semaine 1
- Definition architecture et protocole V1
- Communication RF objet/passerelle

### Semaine 2
- Integration UART et serveur UDP minimal
- Stockage brut + commande getValues()

### Semaine 3
- Application Android (envoi configuration + choix IP/port)
- Affichage OLED pilote par configuration

### Semaine 4
- Communication bidirectionnelle smartphone
- Stabilisation, tests complets, demos

## 12. Livrables attendus
Date demo : 22 mai (matin), 10 minutes max
Date rendu e-campus : 21 mai a 18h max

Le rendu doit inclure :
- Rapport synthetique (au moins 2 pages) + membres de l'equipe + activites realisees
- Application Android documentee
- Code objet (capture + affichage) documente
- Code microcontroleur passerelle documente
- Application serveur documentee

## 13. Criteres de reussite
- Chaine de bout en bout operationnelle (objet -> passerelle -> serveur -> smartphone et retour config)
- Ordre d'affichage configurable a distance
- Donnees correctement stockees et restituees
- Demonstration claire de la valeur technique des choix realises
- Documentation lisible et reproductible

## 14. Conseils pour la soutenance (10 min)
Structure conseillee :
1. Contexte et besoin (1 min)
2. Architecture et protocole (2 min)
3. Demonstration technique en direct (5 min)
4. Limites actuelles et evolutions (1 min)
5. Conclusion (1 min)

Mettre en avant :
- Vos choix d'ingenierie
- Les compromis techniques
- La robustesse de l'implementation
- Ce qui est deja industrialisable et ce qui reste a ameliorer

## 15. Annexe pratique - adaptation a ce repo
### 15.1 Fichier principal
Le comportement passerelle present dans ce projet est implante dans:
- source/main.cpp

Fonctionnement actuel:
- UART a 115200
- Radio activee sur le groupe 10
- Bridge bidirectionnel radio <-> serie

### 15.2 Commandes utiles
Depuis la racine du projet:

```bash
make build
make flash
```

### 15.3 Verification UART
Pour identifier un port serie:

```bash
ls /dev/cu.usbmodem* /dev/ttyACM* 2>/dev/null
```

Pour ouvrir la liaison serie:

```bash
screen /dev/cu.usbmodemXXXXX 115200
```

Sortie de screen:
- Ctrl + A puis Ctrl + K puis y
