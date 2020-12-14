# VerifyLED

## Summary

Things shown in this project:

- The "minimal circuit" required to get a PIC32MX270F256B MPU running and talking to an in-circuit debugger for uploading software and debugging.

- Wiring up and using a digital pin for digital output, controlling an LED.

- Interrupts and interrupt vectors, and initializing a periodic timer for tracking the current time.

## Overview

This simple project tests the connection of a PIC32MX CPU (the [PIC32MX270F256B](https://www.microchip.com/wwwproducts/en/PIC32mx270f256b) CPU) to the [MPLab PICKit 4 In-Circuit Debugger](https://www.microchip.com/Developmenttools/ProductDetails/PG164140).

This consists of a very simple program which pulses a single LED on and off about once a second. This assumes the following basic circuit:

![Schematic](schematic.png)

The circuit block on the left is the absolute minimum circuit (as far as I can determine) necessary to run the PIC32MX270F256B MPU. This (and the configuration code) runs the MPU at 48MHz from an internal oscillator.

I sourced the [PIC32MX270F256B MPUs from DigiKey.](https://www.digikey.com/en/products/detail/microchip-technology/PIC32MX270F256B-50I-SP/4902626) Because I tend to prototype boards rapidly, I also bought a bunch of [28 pin sockets](https://www.amazon.com/gp/product/B07GBP87RF/) because it's cheaper to toss out a failed board with a 16 cent socket than a $4.32 MPU. (Prices as of December 2020.)

The capacitor C2 must be a non-polarized ceramic capacitor; I used one from [this collection of capacitors off Amazon](https://www.amazon.com/gp/product/B07PRC5JJY/). The 10K pull-up resistor can be any 10K-ish resistors, such as one from [this set of resistors.](https://www.amazon.com/dp/B08FHLTV1M/)

(Note: I tend to blow through 10K resistors as pull-ups, so I generally buy them [in sets of 100 to 1000.](https://www.amazon.com/Projects-100EP51410K0-10k-Resistors-Pack/dp/B0185FGYQA/))

The LED and resistors are "suggested"; I generally power the whole thing at 3.3v, and any reasonable resistor from 330 ohms to 470 ohms will do the trick. I also like [these LEDs](https://www.amazon.com/gp/product/B00UWBJM0Q/); they're pretty.

I also put the LED behind a transistor in order to reduce the load on the PIC32MX MPU's pin, but you can probably skip this step and connect the LED and resistor directly to the MPU.

The plug is [one of these guys](https://www.amazon.com/gp/product/B074LK7G86/), and the Eagle board layout accepts one of these. The adapter that plugs into this plug is [one of these.](https://www.amazon.com/gp/product/B07CR2M8W6/)

----

The software itself configures the PIC32 to operate at 48MHz using the internal 8MHz oscillator. 

And this includes the files "timers.h" and "timers.c" which sets Timer 1 on the MPU as a periodic timer that drives an internal interrupt at 100 Hz. This is then used to increment an internal counter.

----

## Main.cpp

The main file shows three things.

First, it shows the list of #pragma config settings which establish basic configuration settings to get the PIC32 processor up and running. These settings establish things like the clock source, CPU clock speed, peripheral clock speed (which can be different than the CPU speed), watchdog settings and the like.

I won't dive too deeply into these settings (I don't really understand all of them myself), but the principle things to know is that these settings currently set the internal CPU clock speed to be driven from the internal oscillator (which runs at 8MHz), and using the PLL multiplier and input dividers, runs the CPU at 48MHz. (The maximum speed the CPU will run at is rated at 50MHz.)

Our internal peripheral clock speed is set at 6MHz. (This becomes important later when setting up the timer, as the timer circuit uses the peripheral clock to operate.)

The second thing it does is initialize our millisecond timer library, so we can ask for a 100 millisecond (1 second) delay. I'll discuss the library below.

The third thing it does is set up pin 2 on the 28-pin DIP as a digital output pin "RA0", sets the pin for output, then writes the value of the pin with an alternate on/off pattern on a 1 second timer. This causes our LED to flash.

The relevant segments of code are:

        ANSELAbits.ANSA0 = 0;   // digital

Our output pin is shared with an analog pin, so we must clear the ANSEL bit for port A on pin 0 in order to use the pin as a digital pin. (Notice that "RA0" is shared with "AN0" on our pin diagram in [this document.](https://ww1.microchip.com/downloads/en/DeviceDoc/PIC32MX1XX2XX%20283644-PIN_Datasheet_DS60001168L.pdf))

        TRISAbits.TRISA0 = 0;   // output

This sets the digital pin to output.

Then we manipulate the pin itself using the LATA latch registers:

        LATAbits.LATA0 = 1; // On

This turns our LED on, and

        LATAbits.LATA0 = 0; // Off

turns off our LED.

## Timer.c/Timer.h

Our timer library sets up timer 1 as a periodic interrupt generator which triggers an interrupt every 1/100th of a second. This shows two features: setting up Timer 1, and setting it up to fire an interrupt in multi-vector interrupt mode.

Discussions for interrupts can be found [here](https://ww1.microchip.com/downloads/en/DeviceDoc/60001108H.pdf), and for the timers [here](https://ww1.microchip.com/downloads/en/DeviceDoc/61105F.pdf).

Note that the interrupt declaration itself is documented in the [XC32 C/C++ Compiler User's Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/50001686J.pdf) at Chapter 14: Interrupts. This both includes the format of the `__ISR` compiler directive, as well as the various `__builtin` functions which handle interrupts.


