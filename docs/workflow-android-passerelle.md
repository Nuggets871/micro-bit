# Workflow final Android, serveur et passerelle micro:bit

## 1. Objet du document
Ce document explique la partie haute du systeme :
- ce qui se passe entre Android et le serveur
- comment le serveur dialogue avec la micro:bit receiver
- quelles commandes et quelles reponses sont utilisees

Le but est de decrire precisement la couche d'integration finale validee.

## 2. Fichiers de reference
Dans le repo `iot-project` :
- `server/serveur.py` : passerelle UDP <-> UART
- `android/app/src/main/java/fr/cpe/iotudpapp/ConnectionActivity.kt` : ecran de connexion
- `android/app/src/main/java/fr/cpe/iotudpapp/MainActivity.kt` : ecran principal
- `android/app/src/main/res/values/strings.xml` : valeurs par defaut IP/port

## 3. Role du serveur
Le serveur est le point de jonction entre le monde embarque et le monde Android.

Il assure quatre fonctions :
1. ouvrir le port serie de la micro:bit receiver
2. lire en continu les lignes `DATA|...` et `CFG-ACK|...`
3. conserver la derniere mesure connue
4. parler en UDP avec Android

## 4. Role de l'application Android
### 4.1 Ecran de connexion
L'ecran de connexion permet de saisir :
- l'IP du serveur
- le port UDP du serveur

Au moment de la validation, l'application :
- envoie un `GET`
- attend une reponse UDP non vide
- ouvre ensuite l'ecran principal si le serveur repond

### 4.2 Ecran principal
L'ecran principal :
- garde une socket UDP persistante ouverte
- envoie `subscribe()` et `GET` au demarrage
- affiche temperature, luminosite, humidite et pression
- affiche le dernier JSON brut recu
- envoie `GET` sur le bouton `Get`
- envoie `TLH`, `LTH` ou `THL` sur les boutons de mode

## 5. Sequence complete de connexion Android
### 5.1 Connexion initiale
1. l'utilisateur saisit IP et port
2. l'application envoie `GET`
3. le serveur repond avec soit la derniere mesure, soit un JSON d'erreur `no_data_yet`
4. l'application considere que le serveur est joignable si une reponse revient
5. l'ecran principal s'ouvre

### 5.2 Abonnement aux mises a jour
Au demarrage de l'ecran principal :
1. Android ouvre une socket UDP
2. Android envoie `subscribe()`
3. Android envoie `GET`
4. le serveur ajoute le client a sa liste d'abonnes
5. chaque nouvelle ligne `DATA|...` lue sur l'UART est convertie en JSON et diffusee aux abonnes

## 6. Messages reseau utilises
### 6.1 Android -> serveur
Messages effectivement utilises par l'UI :
- `GET`
- `subscribe()`
- `TLH`
- `LTH`
- `THL`

Messages egalement supportes par le serveur :
- `unsubscribe()`
- `getStatus()`
- `CFG|TLHP`
- `TLHP`

### 6.2 Serveur -> Android
Le serveur peut emettre :
- `data`
- `config_sent`
- `config_ack`
- `status`
- `subscribed`
- `unsubscribed`
- `error`
- `config_error`

## 7. JSON typiques
### 7.1 Mesure capteur

```json
{
  "type": "data",
  "sequence": 74,
  "temperature": 25,
  "light": 104,
  "humidity": 42,
  "pressure": 996,
  "T": 25,
  "L": 104,
  "H": 42,
  "P": 996
}
```

### 7.2 Commande acceptee par le serveur

```json
{
  "type": "config_sent",
  "order": "TLH"
}
```

### 7.3 ACK de configuration revenu du sender

```json
{
  "type": "config_ack",
  "sequence": 12,
  "order": "TLH"
}
```

## 8. Logs attendus cote serveur
### 8.1 Reception des donnees micro:bit

```text
SERIAL> RX: IOT1|D:74|25|104|42|996|C764A
SERIAL> TX: IOT1|A:74|CE124
SERIAL> DATA T=25C L=104 H=42% P=996hPa
SERIAL> DATA|74|25|104|42|996
```

### 8.2 Activite Android

```text
UDP<10.42.233.42:51234> GET
UDP<10.42.233.42:51234> subscribe()
UDP<10.42.233.42:51234> TLH
```

### 8.3 Retour de configuration

```text
SERIAL> CFG-ACK|12|TLH
```

## 9. Reseau et adressage
### 9.1 Smartphone reel
Sur un smartphone physique, l'application doit utiliser l'adresse IP locale du PC qui lance le serveur.

Exemple reel valide :
- `10.42.233.151`

Cette valeur depend du reseau du moment. Elle doit etre adaptee a chaque environnement.

### 9.2 Emulateur Android
Si l'application tourne dans un emulateur Android local sur la meme machine que le serveur, `10.0.2.2` peut etre utilise comme IP du host.

### 9.3 Port
Le port UDP utilise par defaut est `10000`.

## 10. Valeurs par defaut dans l'application
Les valeurs par defaut de l'ecran de connexion se trouvent dans :
- `android/app/src/main/res/values/strings.xml`

Variables importantes :
- `default_ip`
- `default_port`

L'IP par defaut actuelle `10.0.2.2` est adaptee a l'emulateur, pas a un smartphone reel.
Pour un usage repetitif sur le meme reseau Wi-Fi, cette valeur peut etre modifiee dans `strings.xml`.

## 11. Gestion de l'ordre d'affichage
### 11.1 Ce que le serveur accepte
Le serveur accepte tout ordre valide compose sans doublon parmi `T`, `L`, `H`, `P`, avec longueur 1 a 4.

Exemples valides :
- `TLH`
- `LTH`
- `THL`
- `TLHP`

### 11.2 Ce que l'UI expose aujourd'hui
L'UI Android expose trois presets :
- `TLH`
- `LTH`
- `THL`

Le protocole et le serveur savent faire plus, mais l'interface actuelle reste volontairement simple.

## 12. Depannage Android/serveur
### 12.1 L'ecran de connexion refuse de passer
Verifier :
- l'IP du serveur
- le port `10000`
- que le serveur Python tourne encore
- que le smartphone est sur le meme reseau que le PC

### 12.2 L'ecran principal s'ouvre mais les mesures restent vides
Verifier :
- que le serveur affiche bien des lignes `SERIAL> DATA|...`
- que le serveur voit des messages `UDP<...> subscribe()`
- que la receiver est bien branchee et que la sender reste alimentee

### 12.3 La commande de mode ne fait rien
Verifier :
- qu'un `UDP<...> TLH` ou equivalent apparait dans le serveur
- qu'un `CFG-ACK|...` revient ensuite sur le port serie
- que la configuration demandee est bien valide

## 13. Conclusion
La couche Android <-> serveur <-> receiver est finalisee.
Android ne parle jamais directement au sender. Toute la logique passe par le serveur UDP/UART, ce qui simplifie le systeme et rend le debug reproductible.
