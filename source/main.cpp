#include "MicroBit.h"
#include <stdio.h>

MicroBit uBit;
#define BME280_ADDR (0x76 << 1)

uint16_t dig_T1;
int16_t dig_T2, dig_T3;
int32_t t_fine;
ManagedString mode = "T";
int last_val = 0;
int loop_count = 0; // Compteur pour ralentir l'affichage

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

void initBME280() {
    uBit.sleep(100);
    char config[2] = {0xF4, 0x27}; 
    uBit.i2c.write(BME280_ADDR, config, 2);
    dig_T1 = read16(0x88);
    dig_T2 = (int16_t)read16(0x8A);
    dig_T3 = (int16_t)read16(0x8C);
}

int getTemperatureInt() {
    int v1 = read8(0xFA);
    int v2 = read8(0xFB);
    int v3 = read8(0xFC);
    if (v1 <= 0) return last_val;
    int32_t adc_T = (v1 << 12) | (v2 << 4) | (v3 >> 4);
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    last_val = (t_fine * 5 + 128) >> 8;
    return last_val;
}

int main() {
    uBit.init();
    uBit.serial.baud(115200); 
    initBME280();

    char buffer[64];

    while (1) {
        // 1. Lecture des capteurs
        int valRaw = getTemperatureInt(); 
        int lum = uBit.display.readLightLevel();
        
        // 2. Envoi Serial au Mac (TRÈS RAPIDE)
        int tEnt = valRaw / 100;
        int tDec = (valRaw % 100) / 10;
        sprintf(buffer, "G1:T:%d.%d;L:%d\r\n", tEnt, tDec, lum);
        uBit.serial.send(buffer);

        // 3. Lecture des commandes clavier
        int c = uBit.serial.read(ASYNC); 
        if (c >= 0) {
            char cmd = (char)c;
            if (cmd == 'T' || cmd == 't') mode = "T";
            if (cmd == 'L' || cmd == 'l') mode = "L";
            uBit.display.print(mode); // Affiche juste la lettre pour confirmer
        }

        // 4. Affichage sur les LEDs (Seulement tous les 10 tours pour ne pas bloquer)
        if (loop_count % 10 == 0) {
            if (mode == "T") {
                sprintf(buffer, "%d.%dC", tEnt, tDec);
            } else {
                sprintf(buffer, "L%d", lum);
            }
            // On utilise scroll avec seulement 2 arguments (vitesse 60)
            uBit.display.scroll(buffer, 60);
        }

        loop_count++;
        uBit.sleep(200); // La boucle tourne toutes les 200ms
    }
}