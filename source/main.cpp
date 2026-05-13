#include "MicroBit.h"
#include "build_role.h"

#if BUILD_ROLE == BUILD_ROLE_SENDER

namespace {

const int RADIO_GROUP = 83;
const int RADIO_BAND = 7;
const int RADIO_POWER = 7;
const int ACK_WAIT_MS = 700;
const int POLL_DELAY_MS = 50;
const char *PROTOCOL_PREFIX = "IOT1|";

MicroBit uBit;

void initRadio() {
    uBit.radio.enable();
    uBit.radio.setGroup(RADIO_GROUP);
    uBit.radio.setFrequencyBand(RADIO_BAND);
    uBit.radio.setTransmitPower(RADIO_POWER);
}

void showSendMarker() {
    uBit.display.image.clear();
    uBit.display.image.setPixelValue(0, 2, 255);
}

void showAckMarker() {
    uBit.display.image.clear();
    uBit.display.image.setPixelValue(4, 2, 255);
}

void showErrorMarker() {
    uBit.display.image.clear();
    uBit.display.image.setPixelValue(0, 0, 255);
    uBit.display.image.setPixelValue(4, 0, 255);
    uBit.display.image.setPixelValue(2, 2, 255);
    uBit.display.image.setPixelValue(0, 4, 255);
    uBit.display.image.setPixelValue(4, 4, 255);
}

bool waitForAck(const ManagedString &expectedAck) {
    for (int elapsed = 0; elapsed < ACK_WAIT_MS; elapsed += POLL_DELAY_MS) {
        ManagedString incoming = uBit.radio.datagram.recv();

        if (incoming == expectedAck) {
            uBit.serial.send("RX: ");
            uBit.serial.send(incoming + "\r\n");
            return true;
        }

        if (incoming.length() > 0 && incoming.substring(0, 5) == ManagedString(PROTOCOL_PREFIX)) {
            uBit.serial.send("RX inattendu: ");
            uBit.serial.send(incoming + "\r\n");
        }

        uBit.sleep(POLL_DELAY_MS);
    }

    return false;
}

}

int main() {
    uBit.init();
    uBit.serial.baud(115200);
    initRadio();

    uBit.display.scroll("S");

    int sequence = 0;

    while (1) {
        ManagedString suffix(sequence);
        ManagedString message = ManagedString(PROTOCOL_PREFIX) + ManagedString("PING:") + suffix;
        ManagedString expectedAck = ManagedString(PROTOCOL_PREFIX) + ManagedString("ACK:") + suffix;

        showSendMarker();
        uBit.radio.datagram.send(message);

        uBit.serial.send("TX: ");
        uBit.serial.send(message);
        uBit.serial.send("\r\n");

        if (waitForAck(expectedAck)) {
            showAckMarker();
        } else {
            showErrorMarker();
            uBit.serial.send("ACK non recu\r\n");
        }

        ++sequence;
        uBit.sleep(1000);
    }
}

#endif