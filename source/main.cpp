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

MicroBit uBit;

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
    while (1) {
        // On set le pin2 vert
        uBit.io.P2.setDigitalValue(1);
        uBit.io.P1.setDigitalValue(0);
        uBit.io.P0.setDigitalValue(0);
        uBit.sleep(3000);

        // On set le pin1 jaune
        uBit.io.P2.setDigitalValue(0);
        uBit.io.P1.setDigitalValue(1);
        uBit.io.P0.setDigitalValue(0);
        uBit.sleep(1000);

        // On set le pin0 rouge
        uBit.io.P2.setDigitalValue(0);
        uBit.io.P1.setDigitalValue(0);
        uBit.io.P0.setDigitalValue(1);
        uBit.sleep(3000);
    }



    // If main exits, there may still be other fibers running or registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    release_fiber();
}

