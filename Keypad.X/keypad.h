/*  keypad.h
 *
 *      Scan a keypad system
 */

#ifndef _KEYPAD_H
#define _KEYPAD_H

#include <stdint.h>

/****************************************************************************/
/*																			*/
/*	Class Declaration     													*/
/*																			*/
/****************************************************************************/

/*  Keypad
 *
 *      Keypad scanner class. This wires into the circuit and scans our 4x4
 *  matrix keypad
 */

class Keypad
{
    public:
                            Keypad();
                            ~Keypad();
                            
        void                start();
        void                end();
                            
        uint8_t             getKey();
        
    private:
        uint8_t             lastKey;
};

#endif /* _KEYPAD_H */
