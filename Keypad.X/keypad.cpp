/*  keypad.cpp
 *
 *     Keypad scanner. Note the wiring is a little weird. We have 8 total
 *  lines that run to the keypad:
 * 
 *      Out
 *          RA1
 *          RB2
 *          RB3
 *          RA2
 * 
 *      In
 *          RA3
 *          RB4
 *          RA4
 *          RB5
 * 
 *      We configure the input pins with a pull-up, and use the output pins
 *  to scan, one at a time by pulling down.
 */

#include <xc.h>
#include "keypad.h"
#include "timers.h"

/****************************************************************************/
/*																			*/
/*	Startup/Shutdown     													*/
/*																			*/
/****************************************************************************/

/*  Keypad::Keypad
 *
 *      Construction and startup
 */

Keypad::Keypad()
{
    lastKey = 0;
}

Keypad::~Keypad()
{
    end();
}

/*  Keypad::start
 *
 *      This must be called before calling getKey. This actually starts up
 *  the hardware by initializing the appropriate input/output pins
 */

void Keypad::start()
{
    /*
     *  Configure four digital use of the various lines
     */
    ANSELAbits.ANSA1 = 0;       /* A1 is digital */
    ANSELBbits.ANSB2 = 0;       /* B2 is digital */
    ANSELBbits.ANSB3 = 0;       /* B3 is digital */
    
    /*
     *  Configure outputs
     */
    TRISAbits.TRISA1 = 0;       /* Output */
    TRISAbits.TRISA2 = 0;       /* Output */
    TRISBbits.TRISB2 = 0;       /* Output */
    TRISBbits.TRISB3 = 0;       /* Output */
    
    //ODCAbits.ODCA1 = 1;
    //ODCAbits.ODCA2 = 1;
    //ODCBbits.ODCB2 = 1;
    //ODCBbits.ODCB3 = 1;         /* Configure all pins open drain */
    
    LATAbits.LATA1 = 0;         /* Turn all output pins off */
    LATAbits.LATA2 = 0;
    LATBbits.LATB2 = 0;
    LATBbits.LATB3 = 0;
    
    /* TODO: Configure open-drain */
    /*
     *  Configure inputs
     */
    TRISAbits.TRISA3 = 1;       /* Inputs */
    TRISAbits.TRISA4 = 1;
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB5 = 1;
    
//    CNPUAbits.CNPUA3 = 1;
//    CNPUAbits.CNPUA4 = 1;
//    CNPUBbits.CNPUB4 = 1;
//    CNPUBbits.CNPUB5 = 1;
//    CNPDAbits.CNPDA3 = 0;       /* Configure with weak pull-downs. */
//    CNPDAbits.CNPDA4 = 0;
//    CNPDBbits.CNPDB4 = 0;
//    CNPDBbits.CNPDB5 = 0;
}

/*  Keypad::end
 *
 *      Finish
 */

void Keypad::end()
{
}

/*  Keypad::getKey
 *
 *      Get the key
 */

uint8_t Keypad::getKey()
{
    uint8_t key = 0;
    
    /* Scan row 1 */
    LATAbits.LATA1 = 1;
    DelayMilliseconds(2);
    if (PORTBbits.RB5) key = '1';
    if (PORTAbits.RA4) key = '4';
    if (PORTBbits.RB4) key = '7';
    if (PORTAbits.RA3) key = '*';

    LATAbits.LATA1 = 0;
    LATBbits.LATB2 = 1;
    DelayMilliseconds(2);
    if (PORTBbits.RB5) key = '2';
    if (PORTAbits.RA4) key = '5';
    if (PORTBbits.RB4) key = '8';
    if (PORTAbits.RA3) key = '0';
    
    LATBbits.LATB2 = 0;
    LATBbits.LATB3 = 1;
    DelayMilliseconds(2);
    if (PORTBbits.RB5) key = '3';
    if (PORTAbits.RA4) key = '6';
    if (PORTBbits.RB4) key = '9';
    if (PORTAbits.RA3) key = '#';
    
    LATBbits.LATB3 = 0;
    LATAbits.LATA2 = 1;
    DelayMilliseconds(2);
    if (PORTBbits.RB5) key = 'A';
    if (PORTAbits.RA4) key = 'B';
    if (PORTBbits.RB4) key = 'C';
    if (PORTAbits.RA3) key = 'D';
    
    LATAbits.LATA2 = 0;
    DelayMilliseconds(2);
    
    if (lastKey != key) {
        lastKey = key;
        return key;
    }
    return 0;
}
