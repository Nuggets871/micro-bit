#include "MicroBit.h"
#include "bme280.h"
#include "build_role.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>

#if BUILD_ROLE == BUILD_ROLE_SENDER

namespace {

const int RADIO_GROUP = 83;
const int RADIO_BAND = 7;
const int RADIO_POWER = 7;
const int ACK_WAIT_MS = 700;
const int POLL_DELAY_MS = 50;
const char *PROTOCOL_PREFIX = "IOT1|";
const char *SHARED_SECRET = "MB26";
const char *FIRMWARE_TAG = "S6";
const uint8_t OLED_ADDRESS = SSD130x_ADDR;

MicroBit uBit;
char currentDisplayOrder[8] = "TLHP";
ssd1306 *oledScreen = 0;
int lastTemperature = 0;
int lastLightLevel = 0;
int lastHumidity = -1;
int lastPressure = -1;
bool hasSensorSnapshot = false;

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

bool tryReceiveRadioMessage(ManagedString *message) {
    PacketBuffer packet = uBit.radio.datagram.recv();

    // The DAL uses a 1-byte EmptyPacket sentinel when no radio frame is queued.
    if (packet.length() <= 1) {
        *message = ManagedString();
        return false;
    }

    *message = ManagedString(packet);
    return true;
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

void buildOledMetricLine(char metric, char *lineBuffer, size_t lineBufferSize) {
    switch (metric) {
        case 'T':
            snprintf(lineBuffer, lineBufferSize, "T:%dC", lastTemperature);
            return;
        case 'L':
            snprintf(lineBuffer, lineBufferSize, "L:%d", lastLightLevel);
            return;
        case 'H':
            if (lastHumidity >= 0) {
                snprintf(lineBuffer, lineBufferSize, "H:%d%%", lastHumidity);
            } else {
                snprintf(lineBuffer, lineBufferSize, "H:--");
            }
            return;
        case 'P':
            if (lastPressure >= 0) {
                snprintf(lineBuffer, lineBufferSize, "P:%dhPa", lastPressure);
            } else {
                snprintf(lineBuffer, lineBufferSize, "P:--");
            }
            return;
        default:
            snprintf(lineBuffer, lineBufferSize, "?");
            return;
    }
}

void refreshOledDisplay() {
    if (oledScreen == 0) {
        return;
    }

    oledScreen->clear();
    oledScreen->display_line(0, 0, "ORDRE:");
    oledScreen->display_line(0, 7, currentDisplayOrder);

    if (!hasSensorSnapshot) {
        oledScreen->display_line(2, 0, "Attente capteur");
        oledScreen->update_screen();
        return;
    }

    for (size_t index = 0; index < strlen(currentDisplayOrder) && index < 4; ++index) {
        char lineBuffer[DISPLAY_LINE_LENGTH];
        buildOledMetricLine(currentDisplayOrder[index], lineBuffer, sizeof(lineBuffer));
        oledScreen->display_line(index + 2, 0, lineBuffer);
    }

    oledScreen->update_screen();
}

bool validateDisplayOrder(const char *order) {
    bool seenTemperature = false;
    bool seenLight = false;
    bool seenHumidity = false;
    bool seenPressure = false;
    size_t length = strlen(order);

    if (length == 0 || length > 4) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        switch (order[i]) {
            case 'T':
                if (seenTemperature) return false;
                seenTemperature = true;
                break;
            case 'L':
                if (seenLight) return false;
                seenLight = true;
                break;
            case 'H':
                if (seenHumidity) return false;
                seenHumidity = true;
                break;
            case 'P':
                if (seenPressure) return false;
                seenPressure = true;
                break;
            default:
                return false;
        }
    }

    return true;
}

void applyDisplayOrder(const char *order) {
    strncpy(currentDisplayOrder, order, sizeof(currentDisplayOrder) - 1);
    currentDisplayOrder[sizeof(currentDisplayOrder) - 1] = '\0';
    uBit.serial.send("CFG appliquee: ");
    uBit.serial.send(ManagedString(currentDisplayOrder));
    uBit.serial.send("\r\n");
    refreshOledDisplay();
    uBit.display.scroll(currentDisplayOrder);
}

bool handleControlBody(const char *body) {
    int sequence = 0;
    char order[8];

    if (sscanf(body, "IOT1|G:%d|%7s", &sequence, order) != 2) {
        return false;
    }

    if (!validateDisplayOrder(order)) {
        uBit.serial.send("CFG rejetee: ordre invalide\r\n");
        return true;
    }

    applyDisplayOrder(order);

    char ackBody[32];
    char ackMessage[40];
    snprintf(ackBody, sizeof(ackBody), "%sK:%d|%s", PROTOCOL_PREFIX, sequence, currentDisplayOrder);
    buildAuthenticatedMessage(ackBody, ackMessage, sizeof(ackMessage));
    uBit.radio.datagram.send(ManagedString(ackMessage));
    uBit.serial.send("TX CFG-ACK: ");
    uBit.serial.send(ManagedString(ackMessage));
    uBit.serial.send("\r\n");
    return true;
}

void processPendingRadioMessages() {
    while (true) {
        ManagedString incoming;
        if (!tryReceiveRadioMessage(&incoming)) {
            return;
        }

        if (!hasProtocolPrefix(incoming)) {
            continue;
        }

        char bodyBuffer[32];
        if (!extractValidatedBody(incoming, bodyBuffer, sizeof(bodyBuffer))) {
            uBit.serial.send("RX rejetee: auth invalide\r\n");
            continue;
        }

        if (!handleControlBody(bodyBuffer)) {
            uBit.serial.send("RX ignoree: ");
            uBit.serial.send(ManagedString(bodyBuffer));
            uBit.serial.send("\r\n");
        }
    }
}

bool waitForAck(const char *expectedAckBody) {
    char bodyBuffer[32];

    for (int elapsed = 0; elapsed < ACK_WAIT_MS; elapsed += POLL_DELAY_MS) {
        ManagedString incoming;

        if (!tryReceiveRadioMessage(&incoming)) {
            uBit.sleep(POLL_DELAY_MS);
            continue;
        }

        if (!hasProtocolPrefix(incoming)) {
            uBit.sleep(POLL_DELAY_MS);
            continue;
        }

        if (!extractValidatedBody(incoming, bodyBuffer, sizeof(bodyBuffer))) {
            uBit.serial.send("RX rejetee: auth invalide\r\n");
            uBit.sleep(POLL_DELAY_MS);
            continue;
        }

        if (strcmp(bodyBuffer, expectedAckBody) == 0) {
            uBit.serial.send("RX: ");
            uBit.serial.send(incoming + "\r\n");
            return true;
        }

        if (handleControlBody(bodyBuffer)) {
            uBit.sleep(POLL_DELAY_MS);
            continue;
        }

        if (incoming.length() > 0) {
            uBit.serial.send("RX inattendu: ");
            uBit.serial.send(ManagedString(bodyBuffer));
            uBit.serial.send("\r\n");
        }

        uBit.sleep(POLL_DELAY_MS);
    }

    return false;
}

void readSensors(bme280 *environmentSensor, bool environmentSensorReady, int *temperature, int *lightLevel, int *humidity, int *pressure) {
    *temperature = uBit.thermometer.getTemperature();
    *lightLevel = uBit.display.readLightLevel();
    *humidity = -1;
    *pressure = -1;

    if (!environmentSensorReady) {
        return;
    }

    uint32_t rawPressure = 0;
    int32_t rawTemperature = 0;
    uint16_t rawHumidity = 0;

    if (environmentSensor->sensor_read(&rawPressure, &rawTemperature, &rawHumidity) == 0) {
        *temperature = environmentSensor->compensate_temperature(rawTemperature) / 100;
        *humidity = environmentSensor->compensate_humidity(rawHumidity) / 100;
        *pressure = environmentSensor->compensate_pressure(rawPressure) / 100;
    }
}

}

int main() {
    uBit.init();
    uBit.serial.baud(115200);
    initRadio();

    bme280 environmentSensor(&uBit, &uBit.i2c);
    bool environmentSensorReady = environmentSensor.probe_sensor() == 1;
    ssd1306 oled(&uBit, &uBit.i2c, 0, OLED_ADDRESS);
    oledScreen = &oled;

    uBit.display.scroll(FIRMWARE_TAG);
    uBit.serial.send("BOOT: ");
    uBit.serial.send(ManagedString(FIRMWARE_TAG));
    uBit.serial.send(" GROUP=83 PREFIX=IOT1| MODE=SEC+CFG\r\n");
    uBit.serial.send("CFG ordre initial: ");
    uBit.serial.send(ManagedString(currentDisplayOrder));
    uBit.serial.send("\r\n");
    if (environmentSensorReady) {
        uBit.serial.send("SENSOR: BME280 OK\r\n");
    } else {
        uBit.serial.send("SENSOR: BME280 ABSENT, fallback temp interne\r\n");
    }
    refreshOledDisplay();

    int sequence = 0;

    while (1) {
        int temperature = 0;
        int lightLevel = 0;
        int humidity = -1;
        int pressure = -1;
        char bodyBuffer[32];
        char messageBuffer[40];
        char ackBodyBuffer[24];

        processPendingRadioMessages();
        readSensors(&environmentSensor, environmentSensorReady, &temperature, &lightLevel, &humidity, &pressure);
        lastTemperature = temperature;
        lastLightLevel = lightLevel;
        lastHumidity = humidity;
        lastPressure = pressure;
        hasSensorSnapshot = true;
        refreshOledDisplay();
        snprintf(bodyBuffer, sizeof(bodyBuffer), "%sD:%d|%d|%d|%d|%d", PROTOCOL_PREFIX, sequence, temperature, lightLevel, humidity, pressure);
        snprintf(ackBodyBuffer, sizeof(ackBodyBuffer), "%sA:%d", PROTOCOL_PREFIX, sequence);
        buildAuthenticatedMessage(bodyBuffer, messageBuffer, sizeof(messageBuffer));

        ManagedString message(messageBuffer);

        showSendMarker();
        uBit.radio.datagram.send(message);

        uBit.serial.send("TX: ");
        uBit.serial.send(message);
        uBit.serial.send("\r\n");

        if (waitForAck(ackBodyBuffer)) {
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