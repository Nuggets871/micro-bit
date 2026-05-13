#include "MicroBit.h"
#include "build_role.h"
#include <stdio.h>
#include <string.h>

#if BUILD_ROLE == BUILD_ROLE_RECEIVER

namespace {

const int RADIO_GROUP = 83;
const int RADIO_BAND = 7;
const int RADIO_POWER = 7;
const char *PROTOCOL_PREFIX = "IOT1|";
const char *SHARED_SECRET = "MB26";
const char *FIRMWARE_TAG = "R5";

MicroBit uBit;

uint16_t computeAuthTag(const char *body) {
    uint32_t hash = 2166136261u;

    for (const char *cursor = body; *cursor != '\0'; ++cursor) {
        hash ^= (uint8_t)*cursor;
        hash *= 16777619u;
    }

    for (const char *cursor = SHARED_SECRET; *cursor != '\0'; ++cursor) {
        hash ^= (uint8_t)*cursor;
        hash *= 16777619u;
    }

    return (uint16_t)((hash >> 16) ^ (hash & 0xFFFF));
}

void buildAuthenticatedMessage(const char *body, char *messageBuffer, size_t messageBufferSize) {
    snprintf(messageBuffer, messageBufferSize, "%s|C%04X", body, computeAuthTag(body));
}

bool hasProtocolPrefix(const ManagedString &message) {
    return message.length() >= 5 && strncmp(message.toCharArray(), PROTOCOL_PREFIX, 5) == 0;
}

bool extractValidatedBody(const ManagedString &message, char *bodyBuffer, size_t bodyBufferSize) {
    const char *raw = message.toCharArray();
    const char *tagSeparator = strrchr(raw, '|');

    if (tagSeparator == NULL || tagSeparator[1] != 'C') {
        return false;
    }

    int bodyLength = tagSeparator - raw;
    if (bodyLength <= 0 || bodyLength >= (int)bodyBufferSize) {
        return false;
    }

    memcpy(bodyBuffer, raw, bodyLength);
    bodyBuffer[bodyLength] = '\0';

    unsigned int receivedTag = 0;
    if (sscanf(tagSeparator + 2, "%4x", &receivedTag) != 1) {
        return false;
    }

    return computeAuthTag(bodyBuffer) == (uint16_t)receivedTag;
}

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

bool parseDataBody(const char *body, int *sequence, int *temperature, int *lightLevel, int *humidity, int *pressure) {
    return sscanf(body, "IOT1|D:%d|%d|%d|%d|%d", sequence, temperature, lightLevel, humidity, pressure) == 5;
}

}

int main() {
    uBit.init();
    uBit.serial.baud(115200);
    initRadio();

    uBit.display.scroll(FIRMWARE_TAG);
    uBit.serial.send("BOOT: ");
    uBit.serial.send(ManagedString(FIRMWARE_TAG));
    uBit.serial.send(" GROUP=83 PREFIX=IOT1| MODE=SEC\r\n");

    int lastSequenceSeen = -1;

    while (1) {
        ManagedString msg = uBit.radio.datagram.recv();

        if (hasProtocolPrefix(msg)) {
            char bodyBuffer[32];

            if (!extractValidatedBody(msg, bodyBuffer, sizeof(bodyBuffer))) {
                uBit.serial.send("DROP: auth invalide\r\n");
                uBit.sleep(50);
                continue;
            }

            uBit.serial.send("RX: ");
            uBit.serial.send(msg + "\r\n");

            int sequence = 0;
            int temperature = 0;
            int lightLevel = 0;
            int humidity = 0;
            int pressure = 0;

            if (parseDataBody(bodyBuffer, &sequence, &temperature, &lightLevel, &humidity, &pressure)) {
                char ackBodyBuffer[24];
                char ackBuffer[32];
                char summaryBuffer[48];

                if (sequence == 0) {
                    lastSequenceSeen = -1;
                }

                if (sequence <= lastSequenceSeen) {
                    uBit.serial.send("DROP: replay\r\n");
                    uBit.sleep(50);
                    continue;
                }

                lastSequenceSeen = sequence;

                snprintf(ackBodyBuffer, sizeof(ackBodyBuffer), "%sA:%d", PROTOCOL_PREFIX, sequence);
                buildAuthenticatedMessage(ackBodyBuffer, ackBuffer, sizeof(ackBuffer));
                ManagedString ack(ackBuffer);

                uBit.radio.datagram.send(ack);
                uBit.serial.send("TX: ");
                uBit.serial.send(ack + "\r\n");
                snprintf(summaryBuffer, sizeof(summaryBuffer), "DATA T=%dC L=%d H=%d%% P=%dhPa", temperature, lightLevel, humidity, pressure);
                uBit.serial.send(summaryBuffer);
                uBit.serial.send("\r\n");
                showReceiveMarker();
            }
        }

        uBit.sleep(50);
    }
}

#endif