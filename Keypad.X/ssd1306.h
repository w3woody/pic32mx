/*
 * ssd1306.h
 *
 * Created: 8/8/20 4:45:14 PM
 *  Author: woody
 */ 


#ifndef SSD1306_H_
#define SSD1306_H_

#include "display.h"

/****************************************************************************/
/*																			*/
/*	SSD1306 Hardware														*/
/*																			*/
/****************************************************************************/

/*
 *	Constants
 */

#define SSD1306_I2C_ADDRESS			0x3C		/* SSD1306 I2C Address */

#define SSD1306_MEMORY				1024		/* Total size of display buf */

/****************************************************************************/
/*																			*/
/*	Constants																*/
/*																			*/
/****************************************************************************/

/*
 *  Drawing modes
 */

#define GL_BLACK       				0
#define GL_WHITE       				1
#define GL_XOR         				2

/****************************************************************************/
/*																			*/
/*	SSD1306 Class															*/
/*																			*/
/****************************************************************************/

/*	SSD1306
 *
 *		Class declaration for the SSD1306 display that displays a 128 wide
 *  by 64 tall display.
 */

class SSD1306: public GraphicDisplay
{
	public:
                            SSD1306();
                            ~SSD1306();
                            
        /*
         *  Startup
         */
                            
        bool                start(uint8_t addr = SSD1306_I2C_ADDRESS);
                            
        /*
         *  Standard Stuff
         */
                            
        void                clear();
        bool                writeDisplay();
        
        /*
         *  SSD1306 specific routines
         */
        
		bool                setContrast(uint8_t c);
		bool                setDisplay(bool on);	/* turn display on or off */
        void                setDrawingMode(uint8_t m)
                                {
                                    mode = m;
                                }
        
    protected:
        void                setPixelInternal(uint8_t x, uint8_t y);
        virtual void        setHBarInternal(uint8_t left, uint8_t right, uint8_t y);
        virtual void        setVBarInternal(uint8_t x, uint8_t top, uint8_t bottom);

		/*
		 *	Raw display memory
		 */
		uint8_t             display[SSD1306_MEMORY];/* Display memory */	
		
	private:
        uint8_t             address;
		uint8_t             mode;
};




#endif /* SSD1306_H_ */