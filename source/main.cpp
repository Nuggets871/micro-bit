#include "MicroBit.h"
#include <stdio.h>

MicroBit uBit;

#define BME280_ADDR (0x76 << 1)

uint16_t dig_T1;
int16_t dig_T2, dig_T3;

int readRaw(uint8_t reg) {
    char cmd[1] = {(char)reg};
    char data[1];
    if (uBit.i2c.write(BME280_ADDR, cmd, 1, true) != 0) return -999;
    if (uBit.i2c.read(BME280_ADDR, data, 1) != 0) return -999;
    return (uint8_t)data[0];
}

void initBME280() {
    char ctrl[2] = {0xF4, 0x27};
    uBit.i2c.write(BME280_ADDR, ctrl, 2);
    dig_T1 = readRaw(0x88) | (readRaw(0x89) << 8);
    dig_T2 = (int16_t)(readRaw(0x8A) | (readRaw(0x8B) << 8));
    dig_T3 = (int16_t)(readRaw(0x8C) | (readRaw(0x8D) << 8));
}

int getSimpleTemp() {
    int32_t adc_T = (readRaw(0xFA) << 12) | (readRaw(0xFB) << 4) | (readRaw(0xFC) >> 4);
    if (adc_T < 0) return -999;

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    int32_t t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8; 
}


// commandes pour faire comme putty sous mac :
// ouvrir terminal
// ls /dev/cu.usbmodem*
// -> Cela devrait te retourner quelque chose comme /dev/cu.usbmodem14102. puis:
// screen /dev/cu.usbmodemXXXXX 115200
// Remplacer les XXXX par le numéro trouvé à l'étape precedente
// Pour quitter : Ctrl + A puis Ctrl + K et confirme avec y

int main() {
    uBit.init();
    uBit.serial.baud(115200); // Vitesse de communication
    initBME280();

    uBit.display.scroll("DEBUG MODE");

    while (1) {
        int val = getSimpleTemp();
        int tempC = val / 100;

        // --- ENVOI VERS PUTTY ---
        // On affiche la valeur brute 'val' et la valeur divisée 'tempC'
        uBit.serial.printf("Valeur brute: %d | Temp calculee: %d C\r\n", val, tempC);

        if (val == -999) {
            uBit.display.print("X");
        } else {
            uBit.display.scroll(tempC);
        }
        
        uBit.sleep(2000);
    }
}