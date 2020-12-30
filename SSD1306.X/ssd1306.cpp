/*
 * ssd1306.cpp
 *
 * Created: 8/8/20 4:47:56 PM
 *  Author: woody
 */ 

#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <xc.h>
#include <sys/attribs.h>
#include "ssd1306.h"
#include "i2c.h"
#include "timers.h"

/****************************************************************************/
/*																			*/
/*	Internal Hardware Constants	for the SSD1306								*/
/*																			*/
/****************************************************************************/
/*
 *	Size of display. Notice memory is arranged in column order, meaning each
 *	column is 64-bits of adjacent memory, by 128 columns
 */

#define SSD1306_HEIGHT				64
#define SSD1306_WIDTH				128			/* Width & bytes per page */
#define SSD1306_NUMPAGES			8			/* Bytes in a column */
#define SSD1306_MEMORY				1024		/* Total size of display buf */

/*
 *	SSD1306 command set. (Note: command must start with a preamble of 0x00)
 */

#define SSD1306_SETLOWCOLUMN		0x00		/* Set lower column */
#define SSD1306_SETHIGHCOLUMN		0x10		/* Set higher column */
#define SSD1306_SETMEMORYMODE		0x20		/* Set memory (+1 data) */
#define SSD1306_SETCOLUMNADDRESS	0x21		/* Set column address (+2 data) */
#define SSD1306_SETPAGEADDRESS		0x22		/* Set page address (+2 data) */
#define SSD1306_STOPSCROLLING		0x2E		/* Stop scrolling */
#define SSD1306_SETSTARTLINE		0x40		/* Set display RAM line */
#define SSD1306_SETCONTRAST			0x81		/* Set contrast (+1 data) */
#define SSD1306_SETBRIGHTNESS		0x82		/* Set brightness (+1 data) */
#define SSD1306_CHARGEPUMP			0x8D		/* Charge pump */
#define SSD1306_SETSEGREMAP			0xA0		/* Set segment re-map */
#define SSD1306_SETDISPLAYON_RESUME	0xA4		/* Set display resume */
#define SSD1306_SETDISPLAYON		0xA5		/* Set display on */
#define SSD1306_SETNORMALDISPLAY	0xA6		/* Set normal display */
#define SSD1306_SETINVERSEDISPLAY	0xA7		/* Set inverse display */
#define SSD1306_SETMULTIPLEXRATIO	0xA8		/* Set multiplex ratio (+1 data) */
#define SSD1306_DISPLAYDIM			0xAC		/* Display dim */
#define SSD1306_DISPLAYOFF			0xAE		/* Display off */
#define SSD1306_DISPLAYON			0xAF		/* Display on in normal mode */
#define SSD1306_SETPAGESTART		0xB0		/* Page start */
#define SSD1306_SETCOMSCANINC		0xC0		/* Com mode scan increment */
#define SSD1306_SETCOMSCANDEC		0xC8		/* Com mode scan decrement */
#define SSD1306_SETDISPLAYOFFSET	0xD3		/* Vertical shift (+1 data) */
#define SSD1306_SETDISPLAYCLOCKDIV	0xD5		/* Display clock div (+1 data) */
#define SSD1306_SETPRECHARGE		0xD9		/* Precharge period (+1 data) */
#define SSD1306_SETCOMPINS			0xDA		/* Com pins (+1 data) */
#define SSD1306_SETVCOMLEVEL		0xDB		/* Set Vcomh deselect (+1 data) */

/****************************************************************************/
/*																			*/
/*	Constants																*/
/*																			*/
/****************************************************************************/

/*	GInit
 *
 *		Initialization sequence. Includes DC preamble
 */

static const uint8_t GInit[] = 
{
	0x00,							/* Cmd/Dc preamble */
	SSD1306_DISPLAYOFF,				/* Turn the display off */
	SSD1306_SETDISPLAYCLOCKDIV, 0x80, /* Clock divide value */
	SSD1306_SETMULTIPLEXRATIO, 0x3F,/* 1/64 multiplex ratio (height-1)*/
	SSD1306_SETDISPLAYOFFSET, 0,	/* Display offset to 63 */
	SSD1306_SETSTARTLINE | 0,		/* Display start line */
	SSD1306_CHARGEPUMP, 0x14,		/* Enable charge pump */
	SSD1306_SETMEMORYMODE, 0x02,	/* Page address mode */
	SSD1306_SETSEGREMAP | 0x01,		/* Set segment remap */
	SSD1306_SETCOMSCANDEC,			/* Decrement display column scan */
	SSD1306_SETCOMPINS, 0x12,		/* COM pin alternate com configuration */
	SSD1306_SETCONTRAST, 0x32,		/* Set contrast */
	SSD1306_SETBRIGHTNESS, 0x80,	/* Set brightness */
	SSD1306_SETPRECHARGE, 0xF1,		/* Precharge to Phase 1: 1, Phase 2: F */
	SSD1306_SETVCOMLEVEL, 0x40,		/* VCom detect level */
	SSD1306_SETDISPLAYON_RESUME,
	SSD1306_SETNORMALDISPLAY,
	SSD1306_STOPSCROLLING,			/* Turn off scrolling command */
	SSD1306_DISPLAYON
};

/****************************************************************************/
/*																			*/
/*	SSD1306 Hardware														*/
/*																			*/
/****************************************************************************/

/*	SSD1306::SSD1306
 *
 *		Constructor
 */

SSD1306::SSD1306() : GraphicDisplay((GDSize){ 128, 64 })
{
    mode = GL_WHITE;
}

/*	SSD1306::~SSD1306
 *
 *		Destructor
 */

SSD1306::~SSD1306()
{
}


/*	SSD1306::start
 *
 *		Start up. This sends the startup sequence
 */

bool SSD1306::start(uint8_t addr)
{
    address = addr;
    
    /*
     *  Delay a few milliseconds to make sure the system is stable
     */

	DelayMilliseconds(30);

   	/*
	 *	Copy data to RAM buffer to send to module to start up
	 */

	int8_t err = TWIWrite(address,GInit,sizeof(GInit),1);
	if (err < 0) {
        return false;
	}

	DelayMilliseconds(10);
	setDisplay(true);	
	setContrast(0x2F);
	
	clear();
    
    return true;
}


bool SSD1306::setDisplay(bool on)
{
	uint8_t buffer[2];
	
	buffer[0] = 0;
	buffer[1] = on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF;
	int8_t err = TWIWrite(address,buffer,2,1);
	if (err < 0) {
        return false;
    } else {
        return true;
	}
}


/*	SSD1306::setContrast
 *
 *		Set the contrast
 */

bool SSD1306::setContrast(uint8_t c)
{
	uint8_t buffer[4];
	
	buffer[0] = 0;								/* D/C preamble */
	buffer[1] = SSD1306_SETCONTRAST;			/* Set contrast */
	buffer[2] = c;								/* Value */
	uint8_t err = TWIWrite(address,buffer,3,1);
	if (err < 0) {
        return false;
    } else {
        return true;
	}
}

/*	SSD1306::clear
 *
 *		Clear display memory
 */

void SSD1306::clear()
{
	memset(display,0,SSD1306_MEMORY);
	invalidate();
}

/*	SSD1306::writeDisplay
 *
 *		Write the display memory to the device. This writes in the dirty rectangle
 *	provided. Note right and bottom is equal to the pixel position of the right hand
 *	side, so one pixel would be (0,0)-(0,0)
 */

bool SSD1306::writeDisplay()
{
	uint8_t buffer[32];
	uint8_t pos;
	
	/*
	 *	If size is zero, return
	 */
	
	if ((dirty.size.width == 0) || (dirty.size.height == 0)) {
		validate();
		return true;
	}
	
	/*
	 *	Write the preamble
	 */
	
	uint8_t top = dirty.origin.y / 8;
	uint8_t bottom = 1 + (dirty.origin.y + dirty.size.height) / 8;
	if (bottom > SSD1306_NUMPAGES) {
		bottom = SSD1306_NUMPAGES;
	}
	uint8_t left = dirty.origin.x;
	uint8_t right = dirty.origin.x + dirty.size.width;
	if (right > SSD1306_WIDTH) {
		right = SSD1306_WIDTH;
	}
	
	for (uint8_t p = top; p < bottom; ++p) {
		buffer[0] = 0;			/* dc */
		buffer[1] = SSD1306_SETPAGESTART | (p);
		buffer[2] = SSD1306_SETHIGHCOLUMN | ((left) >> 4);
		buffer[3] = SSD1306_SETLOWCOLUMN | (0x0F & (left));
		
		/* Set start position */
		int8_t err = TWIWrite(address,buffer,4,1);
		if (err < 0) {
            return false;
		}
		
		/* Run the rows */
		pos = 0;
		uint8_t *ptr = display + left + p * SSD1306_WIDTH;
		for (uint8_t x = left; x < right; ++x) {
			if (pos == 0) {
				buffer[pos++] = 0x40;
			}
			buffer[pos++] = *ptr++;
			if (pos >= sizeof(buffer)) {
				int8_t err = TWIWrite(address, buffer, pos, 1);
                if (err < 0) {
                    return false;
                }
				pos = 0;
			}
		}
		
		if (pos > 0) {
			int8_t err = TWIWrite(address, buffer, pos, 1);
            if (err < 0) {
                return false;
            }
		}
	}
	
	validate();
    
    return true;
}


/****************************************************************************/
/*																			*/
/*	SSD1306 Drawing Support													*/
/*																			*/
/****************************************************************************/

/*	SSD1306::setPixel
 *
 *		Set the pixel value. Note the way memory is mapped is in rows
 */

void SSD1306::setPixelInternal(uint8_t x, uint8_t y)
{
	uint8_t bit = 1 << (0x07 & y);
	uint8_t page = (SSD1306_NUMPAGES-1) & (y >> 3);
	
	uint16_t offset = x + page * (uint16_t)SSD1306_WIDTH;
	switch (mode) {
		case GL_BLACK:
			display[offset] &= ~bit;
			break;
		default:
		case GL_WHITE:
			display[offset] |= bit;
			break;
		case GL_XOR:
			display[offset] ^= bit;
			break;
	}
}

/*	SSD1306::setVBarInternal 
 *
 *		Internal routine which draws a vertical bar. Our display architecture
 *	means a vertical bar can be drawn more efficiently than drawing a series
 *	of pixels, and this is used for rectangle drawing. Note bottom is inclusive,
 *	so a 1 pixel high vertical line is drawn when top == bottom
 */

void SSD1306::setVBarInternal(uint8_t x, uint8_t top, uint8_t bottom)
{
	uint16_t offset;
	uint8_t pattern;
	uint8_t i;
	
	uint8_t pageTop = (SSD1306_NUMPAGES-1) & (top >> 3);
	uint8_t pageBottom = (SSD1306_NUMPAGES-1) & (bottom >> 3);
	
	uint8_t topPattern = 0xFF << (top & 0x07);
	uint8_t bottomPattern = (0x02 << (bottom & 0x07)) - 1;
	
	if (pageTop == pageBottom) {
		offset = x + pageTop * (uint16_t)SSD1306_WIDTH;
		pattern = topPattern & bottomPattern;
		switch (mode) {
			case GL_BLACK:
				display[offset] &= ~pattern;
				break;
			default:
			case GL_WHITE:
				display[offset] |= pattern;
				break;
			case GL_XOR:
				display[offset] ^= pattern;
				break;
		}
	} else {
		offset = x + pageTop * (uint16_t)SSD1306_WIDTH;
		switch (mode) {
			case GL_BLACK:
			display[offset] &= ~topPattern;
			break;
			default:
			case GL_WHITE:
			display[offset] |= topPattern;
			break;
			case GL_XOR:
			display[offset] ^= topPattern;
			break;
		}

		offset = x + pageBottom * (uint16_t)SSD1306_WIDTH;
		switch (mode) {
			case GL_BLACK:
				display[offset] &= ~bottomPattern;
				break;
			default:
			case GL_WHITE:
				display[offset] |= bottomPattern;
				break;
			case GL_XOR:
				display[offset] ^= bottomPattern;
				break;
		}
		
		pattern = 0xFF;
		for (i = pageTop + 1; i < pageBottom; ++i) {
			offset = x + i * (uint16_t)SSD1306_WIDTH;
			switch (mode) {
				case GL_BLACK:
					display[offset] &= ~pattern;
					break;
				default:
					case GL_WHITE:
					display[offset] |= pattern;
				break;
					case GL_XOR:
					display[offset] ^= pattern;
				break;
			}
		}
	}
}

/*
 *	This is a little faster than calling setPixel for a row. Note that right is
 *	inclusive, so left == right draws 1 pixel.
 */

void SSD1306::setHBarInternal(uint8_t left, uint8_t right, uint8_t y)
{
	uint8_t bit = 1 << (0x07 & y);
	uint8_t page = (SSD1306_NUMPAGES-1) & (y >> 3);
	uint8_t i;
	
	uint16_t offset = left + page * (uint16_t)SSD1306_WIDTH;
	i = left;
	do {
		switch (mode) {
			case GL_BLACK:
				display[offset] &= ~bit;
				break;
			default:
			case GL_WHITE:
				display[offset] |= bit;
				break;
			case GL_XOR:
				display[offset] ^= bit;
				break;
		}
		
		++offset;
	} while (i++ < right);
}
