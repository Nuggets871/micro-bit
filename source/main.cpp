#include "MicroBit.h"
#include <stdio.h>

MicroBit uBit;

#define BME280_ADDR (0x76 << 1)

// Calibration
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
int32_t t_fine;
int last_val = 0;

// --- I2C ---
int read8(uint8_t reg) {
    char cmd[1] = {(char)reg};
    char data[1];

    if (uBit.i2c.write(BME280_ADDR, cmd, 1, true) != 0) return -999;
    if (uBit.i2c.read(BME280_ADDR, data, 1) != 0) return -999;

    return (uint8_t)data[0];
}

uint16_t read16(uint8_t reg) {
    int lo = read8(reg);
    int hi = read8(reg + 1);

    if (lo < 0 || hi < 0) return 0;

    return (uint16_t)lo | ((uint16_t)hi << 8);
}

// --- INIT CAPTEUR ---
void initBME280() {
    uBit.sleep(100);

    char config[2] = {0xF4, 0x27};
    uBit.i2c.write(BME280_ADDR, config, 2);

    dig_T1 = read16(0x88);
    dig_T2 = (int16_t)read16(0x8A);
    dig_T3 = (int16_t)read16(0x8C);
}

// --- TEMP ---
int getTemperatureInt() {
    int v1 = read8(0xFA);
    int v2 = read8(0xFB);
    int v3 = read8(0xFC);

    if (v1 <= 0) return last_val;

    int32_t adc_T = (v1 << 12) | (v2 << 4) | (v3 >> 4);

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                    ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    last_val = (t_fine * 5 + 128) >> 8;

    return last_val;
}

// --- MAIN ---
int main() {
    uBit.init();

    // Configuration radio
    uBit.radio.enable();
    uBit.radio.setGroup(10);
    uBit.radio.setFrequencyBand(7);
    uBit.radio.setTransmitPower(7);

    initBME280();

    uBit.display.scroll("OBJ");

    char buffer[64];

    while (1) {
        int valRaw = getTemperatureInt();
        int lum = uBit.display.readLightLevel();

        int tEnt = valRaw / 100;
        int tDec = (valRaw % 100) / 10;

        // Format message
        sprintf(buffer, "G1:T:%d.%d;L:%d", tEnt, tDec, lum);

        // Envoi radio
        uBit.radio.datagram.send(buffer);

        // Debug USB
        uBit.serial.send("ENVOI: ");
        uBit.serial.send(buffer);
        uBit.serial.send("\r\n");

        // Flash LED
        uBit.display.image.setPixelValue(2, 2, 255);
        uBit.sleep(100);
        uBit.display.image.setPixelValue(2, 2, 0);

        uBit.sleep(900);
    }
}