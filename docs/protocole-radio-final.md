# Protocoles finaux - radio, UART et UDP

## 1. Objet du document
Ce document formalise la specification finale des echanges utilises dans le projet :
- radio entre sender et receiver
- UART entre receiver et PC
- UDP entre serveur et Android

Il complete le guide pratique en decrivant le protocole de facon normative.

## 2. Versions validees
- firmware sender : `S6`
- firmware receiver : `R6`
- serveur PC : `iot-project/server/serveur.py`
- application Android : `iot-project/android`

## 3. Parametres communs
### 3.1 Radio micro:bit
- groupe radio : `83`
- bande radio : `7`
- puissance radio : `7`

### 3.2 Prefixe et securite
- prefixe de protocole : `IOT1|`
- secret partage : `MB26`
- tag d'integrite : suffixe `|CXXXX`

Le tag est calcule a partir du corps du message et du secret partage avec une variante simple de FNV-1a sur 16 bits.
Cette protection vise l'integrite et l'authentification legere, pas un chiffrement fort.

## 4. Donnees transportees
Le systeme transporte quatre mesures :
- temperature en degres Celsius
- luminosite lue sur la micro:bit
- humidite relative en pourcentage
- pression en hPa

Si le capteur BME280 n'est pas disponible :
- temperature = capteur interne micro:bit
- humidite = -1
- pression = -1

## 5. Couche radio
### 5.1 Trame de donnees sender -> receiver
Corps :

```text
IOT1|D:<sequence>|<temperature>|<light>|<humidity>|<pressure>
```

Trame finale :

```text
IOT1|D:69|25|98|42|996|CE90B
```

### 5.2 Trame d'ACK receiver -> sender
Corps :

```text
IOT1|A:<sequence>
```

Trame finale :

```text
IOT1|A:69|C1AA3
```

### 5.3 Trame de configuration receiver -> sender
Corps :

```text
IOT1|G:<sequence>|<order>
```

Exemple :

```text
IOT1|G:12|TLHP|CXXXX
```

### 5.4 ACK de configuration sender -> receiver
Corps :

```text
IOT1|K:<sequence>|<order>
```

Exemple :

```text
IOT1|K:12|TLHP|CXXXX
```

## 6. Regles de validation radio
### 6.1 Cote receiver
Une trame de donnees n'est acceptee que si :
- le prefixe `IOT1|` est present
- le suffixe `|CXXXX` est present
- le tag recalcule correspond au tag recu
- la sequence n'est pas un replay simple

En cas d'erreur :
- tag invalide -> `DROP: auth invalide`
- replay -> `DROP: replay`

### 6.2 Cote sender
Un ACK de donnees est accepte seulement si :
- le prefixe est correct
- le tag est valide
- le corps correspond exactement a `IOT1|A:<sequence>`

Une trame `G:` de configuration est acceptee seulement si :
- le tag est valide
- l'ordre demande est compose d'une permutation valide de `T`, `L`, `H`, `P`

## 7. Couche UART receiver <-> PC
### 7.1 Lignes machine receiver -> PC
Ligne de donnees :

```text
DATA|69|25|98|42|996
```

Ligne d'ACK de configuration :

```text
CFG-ACK|12|TLHP
```

Ligne d'erreur de configuration :

```text
CFG-ERR|ordre invalide
```

### 7.2 Lignes de debug receiver -> PC
Le receiver emet aussi des lignes humaines utiles pour le debug :

```text
RX: IOT1|D:69|25|98|42|996|CE90B
TX: IOT1|A:69|C1AA3
DATA T=25C L=98 H=42% P=996hPa
```

### 7.3 Commandes PC -> receiver
Commande principale :

```text
CFG|TLHP
```

Le receiver accepte aussi directement un ordre brut comme `TLHP`.

## 8. Couche UDP serveur <-> Android
### 8.1 Commandes supportees par le serveur
Le serveur accepte :
- `GET`
- `getValues()`
- `GET_VALUES`
- `subscribe()`
- `unsubscribe()`
- `getStatus()`
- `GET_STATUS`
- un ordre brut comme `TLH`, `LTH`, `THL`, `TLHP`
- un ordre prefixe comme `CFG|TLHP`

### 8.2 Reponse `data`

```json
{
  "type": "data",
  "sequence": 69,
  "temperature": 25,
  "light": 98,
  "humidity": 42,
  "pressure": 996,
  "T": 25,
  "L": 98,
  "H": 42,
  "P": 996
}
```

Le serveur fournit volontairement les noms longs et les clefs courtes pour rester compatible avec l'application Android adaptee.

### 8.3 Reponse `config_sent`

```json
{
  "type": "config_sent",
  "order": "TLH"
}
```

### 8.4 Reponse `config_ack`

```json
{
  "type": "config_ack",
  "sequence": 12,
  "order": "TLH"
}
```

### 8.5 Reponse `status`

```json
{
  "type": "status",
  "latest_data": {
    "type": "data",
    "sequence": 69,
    "temperature": 25,
    "light": 98,
    "humidity": 42,
    "pressure": 996,
    "T": 25,
    "L": 98,
    "H": 42,
    "P": 996
  },
  "latest_config_ack": {
    "sequence": 12,
    "order": "TLH"
  },
  "subscribers": 1
}
```

### 8.6 Reponses d'etat ou d'erreur

```json
{"type": "subscribed"}
{"type": "unsubscribed"}
{"type": "error", "message": "no_data_yet"}
{"type": "error", "message": "unsupported_command"}
{"type": "config_error", "message": "ordre invalide"}
```

## 9. Commandes exposees par l'application Android
L'application Android finale expose dans son interface :
- le bouton `Get` -> envoie `GET`
- les boutons `TLH`, `LTH`, `THL`

Le serveur supporte aussi `TLHP`, meme si cette commande n'est pas exposee par un bouton dans l'UI actuelle.

## 10. Messages de boot attendus
### 10.1 Sender

```text
BOOT: S6 GROUP=83 PREFIX=IOT1| MODE=SEC+CFG
CFG ordre initial: TLHP
SENSOR: BME280 OK
```

### 10.2 Receiver

```text
BOOT: R6 GROUP=83 PREFIX=IOT1| MODE=SEC+UART
UART commandes: CFG|TLHP
```

## 11. Notes d'implementation importantes
### 11.1 Bluetooth desactive
`config.json` desactive explicitement le Bluetooth. C'est indispensable pour utiliser la radio datagram micro:bit de facon fiable.

### 11.2 Piege DAL corrige dans le sender
Le DAL micro:bit renvoie un `EmptyPacket` de longueur `1` quand aucune trame radio n'est disponible.
Si on convertit directement ce paquet en `ManagedString`, il peut etre pris a tort pour un message recu.

Le firmware sender final filtre explicitement ce sentinel avant tout traitement radio.
Sans cette correction, le sender pouvait se bloquer dans sa boucle avant meme le premier `TX:`.

## 12. Validation observee
Exemple de fonctionnement reel valide :

```text
TX: IOT1|D:69|25|98|42|996|CE90B
RX: IOT1|A:69|C1AA3
```

```text
RX: IOT1|D:69|25|98|42|996|CE90B
TX: IOT1|A:69|C1AA3
DATA T=25C L=98 H=42% P=996hPa
DATA|69|25|98|42|996
```

## 13. Limites connues
- pas d'identifiant d'objet
- anti-replay simple
- pas de chiffrement fort
- secret partage code en dur
- l'application Android n'expose pas encore un bouton `TLHP`
- des valeurs capteur aberrantes peuvent apparaitre ponctuellement et pourraient etre filtrees cote serveur
