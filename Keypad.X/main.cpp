/*  main.cpp
 *
 *      Main entry point for a simple program to flash an LED attached to R0
 */


// PIC32MX270F256B Configuration Bit Settings

// 'C' source line config statements

// DEVCFG3
#pragma config USERID = 0xFFFF          // Enter Hexadecimal value (Enter Hexadecimal value)
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = ON            // USB USID Selection (Controlled by the USB Module)
#pragma config FVBUSONIO = ON           // USB VBUS ON Selection (Controlled by USB Module)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_24         // PLL Multiplier (15x Multiplier)
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider (2x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_2         // System PLL Output Clock Divider (PLL Divide by 1)

// DEVCFG1
#pragma config FNOSC = FRCPLL           // Oscillator Selection Bits (Fast RC Osc with PLL)
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window Size is 25%)

// DEVCFG0
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Port Disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "timers.h"
#include "i2c.h"
#include "ssd1306.h"
#include "fonts.h"
#include "keypad.h"

static SSD1306 display;
static Keypad keypad;

int main()
{
    InitMillisecondTimer();
    TWIInit(TWI_FREQ);
    
    ANSELAbits.ANSA0 = 0;   // digital
    TRISAbits.TRISA0 = 0;   // output
    
    display.start();
    display.clear();
    display.moveTo((GDPoint){0,10});
    display.setFont(&smallfont);
    display.drawString("Hello there!");
    
//    display.frameOval((GDRect){ 20, 25, 50, 35 });
//    display.paintOval((GDRect){ 25, 30, 40, 25 });
//    
//    for (uint8_t x = 0; x < 127; x += 8) {
//        display.moveTo((GDPoint){28,30});
//        display.lineTo((GDPoint){x,63});
//    }
//    display.moveTo((GDPoint){28,30});
//    display.lineTo((GDPoint){127,63});
    display.writeDisplay();
         
    uint8_t xpos = 0;
    keypad.start();
    for (;;) {
        uint8_t c = keypad.getKey();
        
        if (c) {
            if (xpos >= 120) {
                xpos = 0;
                display.setDrawingMode(GL_BLACK);
                display.paintRect((GDRect){0,10,128,10});
            }
            
            display.setDrawingMode(GL_WHITE);
            display.moveTo((GDPoint){xpos,20});
            display.drawChar(c);
            display.writeDisplay();
            
            xpos += 6;
        }
        
//        DelayMilliseconds(10);
//        LATAbits.LATA0 = 1;
//        DelayMilliseconds(10);
//        LATAbits.LATA0 = 0;
    }
}
