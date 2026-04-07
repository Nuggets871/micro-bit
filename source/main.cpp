#include "MicroBit.h"

MicroBit uBit;

void onData(MicroBitEvent) {
    ManagedString s = uBit.radio.datagram.recv();
    // On ajoute un message de debug pour être sûr
    uBit.serial.send("PAQUET RECU: ");
    uBit.serial.send(s + "\r\n");
    
    uBit.display.print("!"); // Affiche un point d'exclamation à chaque réception
}

int main() {
    uBit.init();
    uBit.serial.baud(115200);
    
    uBit.radio.enable();
    uBit.radio.setGroup(10); // <--- VERIFIE BIEN QUE C'EST 10 SUR LES DEUX
    
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);

    uBit.display.scroll("GW-V2");

    while (1) {
        // On envoie un petit message au Mac toutes les 2 sec pour prouver que le câble marche
        uBit.serial.send("En attente de radio...\r\n");
        uBit.sleep(2000);
    }
}