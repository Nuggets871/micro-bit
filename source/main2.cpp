#include "MicroBit.h"
#include "build_role.h"

#if BUILD_ROLE == BUILD_ROLE_RECEIVER

namespace {

const int RADIO_GROUP = 83;
const int RADIO_BAND = 7;
const int RADIO_POWER = 7;
const char *PROTOCOL_PREFIX = "IOT1|";

MicroBit uBit;

void initRadio() {
    uBit.radio.enable();
    uBit.radio.setGroup(RADIO_GROUP);
    uBit.radio.setFrequencyBand(RADIO_BAND);
    uBit.radio.setTransmitPower(RADIO_POWER);
}

void showReceiveMarker() {
    uBit.display.image.clear();
    uBit.display.image.setPixelValue(2, 0, 255);
    uBit.display.image.setPixelValue(2, 1, 255);
    uBit.display.image.setPixelValue(2, 2, 255);
    uBit.display.image.setPixelValue(2, 3, 255);
    uBit.display.image.setPixelValue(2, 4, 255);
}

}

int main() {
    uBit.init();
    uBit.serial.baud(115200);
    initRadio();

    uBit.display.scroll("R");

    while (1) {
        ManagedString msg = uBit.radio.datagram.recv();

        if (msg.length() > 0 && msg.substring(0, 5) == ManagedString(PROTOCOL_PREFIX)) {
            uBit.serial.send("RX: ");
            uBit.serial.send(msg + "\r\n");

            if (msg.length() > 10 && msg.substring(5, 10) == "PING:") {
                ManagedString suffix = msg.substring(10, msg.length());
                ManagedString ack = ManagedString(PROTOCOL_PREFIX) + ManagedString("ACK:") + suffix;

                uBit.radio.datagram.send(ack);
                uBit.serial.send("TX: ");
                uBit.serial.send(ack + "\r\n");
                showReceiveMarker();
            }
        }

        uBit.sleep(50);
    }
}

#endif