/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "MicroBit.h"
#include "neopixel.h"

MicroBit uBit;

// ======================
// CONFIG I2C
// ======================
#define OLED_ADDR  0x3C
#define BME280_ADDR 0x76

// ======================
// OLED
// ======================
void writeCommand(uint8_t cmd) {
    char data[2] = {0x00, (char)cmd};
    uBit.i2c.write(OLED_ADDR << 1, data, 2);
}

void writeData(uint8_t dataByte) {
    char data[2] = {0x40, (char)dataByte};
    uBit.i2c.write(OLED_ADDR << 1, data, 2);
}

void initOLED() {
    writeCommand(0xAE);
    writeCommand(0xA6);
    writeCommand(0x20);
    writeCommand(0x00);
    writeCommand(0xAF);
}

void setCursor(int page, int col) {
    writeCommand(0xB0 + page);
    writeCommand(0x00 + (col & 0x0F));
    writeCommand(0x10 + ((col >> 4) & 0x0F));
}

void displayText(const char* text) {
    setCursor(0, 0);
    while (*text) {
        writeData(*text++);
    }
}

// ======================
// BME280
// ======================
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
int32_t t_fine;

uint8_t read8(uint8_t reg) {
    char cmd[1] = {(char)reg};
    char data[1];
    uBit.i2c.write(BME280_ADDR << 1, cmd, 1, true);
    uBit.i2c.read(BME280_ADDR << 1, data, 1);
    return data[0];
}

uint16_t read16(uint8_t reg) {
    return read8(reg) | (read8(reg + 1) << 8);
}

int16_t readS16(uint8_t reg) {
    return (int16_t)read16(reg);
}

void readCalibration() {
    dig_T1 = read16(0x88);
    dig_T2 = readS16(0x8A);
    dig_T3 = readS16(0x8C);
}

void initBME280() {
    char ctrl_hum[2] = {0xF2, 0x01};
    uBit.i2c.write(BME280_ADDR << 1, ctrl_hum, 2);

    char ctrl_meas[2] = {0xF4, 0x27};
    uBit.i2c.write(BME280_ADDR << 1, ctrl_meas, 2);

    readCalibration();
}

int32_t readTemperatureBME280() {
    int32_t adc_T = (read8(0xFA) << 12) | (read8(0xFB) << 4) | (read8(0xFC) >> 4);

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                     ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                     ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8; // °C * 100
}

int main()
{
    // Initialise the micro:bit runtime.
    uBit.init();


    //int count = 0;

    // Insert your code here!
    //uBit.display.scroll("HELLO WORLD! :)");
    // uBit.compass.calibrate();

    // while(1) {
    //     if (uBit.buttonA.isPressed()) {
    //         uBit.display.scroll("A");
    //     }
    //     if (uBit.buttonB.isPressed()) {
    //         uBit.display.scroll("B");
    //     }

        //int temp = uBit.thermometer.getTemperature();
        //uBit.display.scroll(temp);

        //if (uBit.accelerometer.getX() < -200) {
        //    count++;
        //}
        //uBit.display.scroll(count);

    //      int heading = uBit.compass.heading();

    //     uBit.display.clear();

    //     if (heading > 315 || heading <= 45) {
    //         // Nord
    //         uBit.display.image.setPixelValue(2, 0, 255);
    //     }
    //     else if (heading <= 135) {
    //         // Est
    //         uBit.display.image.setPixelValue(4, 2, 255);
    //     }
    //     else if (heading <= 225) {
    //         // Sud
    //         uBit.display.image.setPixelValue(2, 4, 255);
    //     }
    //     else {
    //         // Ouest
    //         uBit.display.image.setPixelValue(0, 2, 255);
    //     }

    //     uBit.sleep(200);
    // }


// TP2 - Exercice 1
    // while (1) {
    //     // On set le pin2 vert
    //     uBit.io.P2.setDigitalValue(1);
    //     uBit.io.P1.setDigitalValue(0);
    //     uBit.io.P0.setDigitalValue(0);
    //     uBit.sleep(3000);

    //     // On set le pin1 jaune
    //     uBit.io.P2.setDigitalValue(0);
    //     uBit.io.P1.setDigitalValue(1);
    //     uBit.io.P0.setDigitalValue(0);
    //     uBit.sleep(1000);

    //     // On set le pin0 rouge
    //     uBit.io.P2.setDigitalValue(0);
    //     uBit.io.P1.setDigitalValue(0);
    //     uBit.io.P0.setDigitalValue(1);
    //     uBit.sleep(3000);
    // }

// TP2 - exo 2
    // neopixel_strip_t strip;
    // neopixel_init(&strip, (uint8_t)MICROBIT_PIN_P0, 1); // 1 LED NeoPixel sur P0
    // uBit.sleep(10); // Laisse la ligne DATA se stabiliser apres init.

    // while (1) {
    //     // Bleu
    //     neopixel_set_color(&strip, 0, 0, 0, 255);
    //     neopixel_show(&strip);
    //     uBit.sleep(250);

    //     // Blanc
    //     neopixel_set_color(&strip, 0, 255, 255, 255);
    //     neopixel_show(&strip);
    //     uBit.sleep(250);

    //     // Rouge
    //     neopixel_set_color(&strip, 0, 255, 0, 0);
    //     neopixel_show(&strip);
    //     uBit.sleep(250);
    // }

// TP2 - exo 3
    // uBit.init();

    // while (1) {
    //     int temp = uBit.thermometer.getTemperature();
    //     int light = uBit.display.readLightLevel();

    //     uBit.serial.printf("Temp: %d C\n", temp);
    //     uBit.serial.printf("Lumiere: %d\n", light);

    //     uBit.sleep(1000);
    // }

    // On utilise putty pour voir le retour du port serial



// TP2 - exo 4
    // uBit.init();

    // while (1) {
    //     int temp = uBit.thermometer.getTemperature();
    //     int light = uBit.display.readLightLevel();

    //     // Simulation capteurs (si pas de driver)
    //     int pressure = 1000 + (temp % 10);
    //     int humidity = 40 + (temp % 20);
    //     int uv = light / 50;

    //     uBit.serial.printf("Temp: %d C\n", temp);
    //     uBit.serial.printf("Pressure: %d hPa\n", pressure);
    //     uBit.serial.printf("Humidity: %d %%\n", humidity);
    //     uBit.serial.printf("Lumiere: %d lx\n", light);
    //     uBit.serial.printf("UV: %d\n\n", uv);

    //     uBit.sleep(2000);
    // }



// TP2 - exo 5

    initOLED();
    initBME280();

    char buffer[32];

    while (1) {
        // Températures
        int tempMicro = uBit.thermometer.getTemperature();
        float tempBME = readTemperatureBME280() / 100.0;

        // --- Affichage noms ---
        displayText("Groupe:");
        uBit.sleep(2000);

        displayText("Ali & Sara");
        uBit.sleep(2000);

        // --- Temp micro:bit ---
        sprintf(buffer, "T micro: %dC", tempMicro);
        displayText(buffer);
        uBit.sleep(2000);

        // --- Temp BME280 ---
        sprintf(buffer, "T meteo: %.2fC", tempBME);
        displayText(buffer);
        uBit.sleep(2000);

        // --- Comparaison ---
        float diff = tempMicro - tempBME;
        sprintf(buffer, "Diff: %.2fC", diff);
        displayText(buffer);
        uBit.sleep(3000);
    }

    // If main exits, there may still be other fibers running or registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    release_fiber();
}

