#include "MicroBit.h"

MicroBit uBit;

int main() {
    uBit.init();
    uBit.serial.baud(115200);

    // Configuration radio
    uBit.radio.enable();
    uBit.radio.setGroup(10);
    uBit.radio.setFrequencyBand(7);
    uBit.radio.setTransmitPower(7);

    uBit.display.scroll("GW");

    while (1) {
        // Réception radio
        ManagedString msg = uBit.radio.datagram.recv();

        if (msg.length() > 0) {
            // Debug USB
            uBit.serial.send("RECU: ");
            uBit.serial.send(msg + "\r\n");

            // Feedback visuel
            uBit.display.print("!");
        } else {
            // Debug pour vérifier que ça tourne
            uBit.serial.send("... attente radio ...\r\n");
        }

        uBit.sleep(500);
    }
}