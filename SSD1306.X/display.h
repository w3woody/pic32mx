/*  display.h
 *
 *      The core display class. This contains all the common drawing code used
 *  for graphical displays, including line drawing and font drawing routines.
 * 
 *      This class should be overridden with hardware specific code that 
 *  (minimally) implements the setPixel routines. 
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

/****************************************************************************/
/*																			*/
/*	Graphical Structures													*/
/*																			*/
/****************************************************************************/

/*	GDPoint
 *
 *		2D point
 */

struct GDPoint {
	uint8_t x,y;	
};

/*	GDSize
 *
 *		2D size
 */

struct GDSize {
	uint8_t width,height;	
};

/*	Rect
 *
 *		Rectangular area
 */

struct GDRect {
	GDPoint origin;
	GDSize size;
};

/****************************************************************************/
/*																			*/
/*	Font Structures     													*/
/*																			*/
/****************************************************************************/

/*
 *  This is the format of a font record. This uses the same font format as
 *  the Adafruit font structures
 * 
 *  More information can be found at:
 * 
 *  https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format/
 */

/*  GFXglyph
 *
 *      The font data stored per glyph
 */

typedef struct GFXglyph {
	uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
	uint8_t width;         ///< Bitmap dimensions in pixels
	uint8_t height;        ///< Bitmap dimensions in pixels
	uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
	int8_t xOffset;        ///< X dist from cursor pos to UL corner
	int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/*  GFXfont
 *
 *      The font data stored for the font as a whole. This is an array of
 *  glyphs which refer to offsets inside the bitmap data
 */

typedef struct GFXfont {
	const uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
	const GFXglyph *glyph;  ///< Glyph array
	uint16_t first;   ///< ASCII extents (first char)
	uint16_t last;    ///< ASCII extents (last char)
	uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

/****************************************************************************/
/*																			*/
/*	Graphical Display														*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay
 *
 *      This contains standard code for drawing various objects on our screen.
 *  Note the core class does not know about pixel color, XOR drawing mode or
 *  the like. That should be handled by higher-level code.
 * 
 *      Note that this also assumes an 8-bit coordinate system, and is useful
 *  for very small displays, such as those controlled by the SSD1305.
 */

class GraphicDisplay
{
    public:
                            GraphicDisplay(GDSize size);
        virtual             ~GraphicDisplay();

        /*
         *  Required basic routines
         */
                           
        virtual void        clear() = 0;
        virtual bool        writeDisplay() = 0;
        
        /*
         *  Dirty rectangle management. Internally we track the largest rectangle
         *  of data that may have been invalidated during drawing. This can then
         *  be used by a class that derives from this class to minimize I/O
         *  operations to write the area of the display that has changed.
         */
       
        void                invalidate();
        void                validate();
        
        /*
         *  Font management
         */
       
		void                setFont(const GFXfont *f)
                                {
                                    font = f;
                                }

		uint8_t             width(uint16_t c);
		uint8_t             width(const char *text);
        
        /*
         *  Basic drawing routines
         */ 
        
        void                setPixel(uint8_t x, uint8_t y);
        
        /*
         *  Routines that use the above to render
         */
        
		void                moveTo(GDPoint pt)
                                {
                                    pos = pt;
                                }
		void                lineTo(GDPoint pt);
		
		void                drawString(const char *text);
		void                drawChar(uint16_t c);
		
		void                paintRect(GDRect r);
		void                frameRect(GDRect r);
        void                paintOval(GDRect r);
        void                frameOval(GDRect r);
        void                paintRoundRect(GDRect r, uint8_t corner);
        void                frameRoundRect(GDRect r, uint8_t corner);
               
    protected:
        /*
         *  Routines that could be overridden to run faster
         */
       
        virtual void        setPixelInternal(uint8_t x, uint8_t y) = 0;
        virtual void        setHBarInternal(uint8_t left, uint8_t right, uint8_t y);
        virtual void        setVBarInternal(uint8_t x, uint8_t top, uint8_t bottom);

    private:
        void                markDirty(uint8_t left, uint8_t top, uint8_t width, uint8_t height);
        void                drawCorner(uint8_t x, uint8_t y, uint8_t r, uint8_t cmask);
    
    protected:
        const GFXfont       *font;
        
        GDPoint             pos;
        GDSize              size;
        GDRect              dirty;
};


#endif /* __DISPLAY_H__ */
