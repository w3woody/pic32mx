/*  display.cpp
 *
 *      Core graphic display drawing code.
 * 
 *      Some of the algorithms are derived from: "A Rasterizing Algorithm for 
 *  Drawing Curves", Alois, Zingl.
 * 
 *  Source: http://members.chello.at/~easyfilter/Bresenham.pdf
 * 
 *  http://members.chello.at/~easyfilter/bresenham.html
 */

#include <stdlib.h>
#include <stdint.h>

#include "display.h"

/****************************************************************************/
/*																			*/
/*	Construction/Destruction												*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::GraphicDisplay
 *
 *      Graphic display construction
 */

GraphicDisplay::GraphicDisplay(GDSize s) : size(s)
{
    pos = (GDPoint){ 0, 0 };
    invalidate();
}

/*  GraphicDisplay::~GraphicDisplay
 *
 *      Destruction
 */

GraphicDisplay::~GraphicDisplay()
{
}

/****************************************************************************/
/*																			*/
/*	Default Horizontal/Vertical												*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::setHBar
 * 
 *      Internal horizontal bar drawing
 */

void GraphicDisplay::setHBarInternal(uint8_t left, uint8_t right, uint8_t y)
{
    for (uint8_t i = left; i <= right; ++i) {
        setPixelInternal(i,y);
    }
}

/*  GraphicDisplay::setVBar
 *
 *      Internal vertical bar drawing
 */

void GraphicDisplay::setVBarInternal(uint8_t x, uint8_t top, uint8_t bottom)
{
    for (uint8_t i = top; i <= bottom; ++i) {
        setPixelInternal(x,i);
    }
}

/****************************************************************************/
/*																			*/
/*	Dirty Rectangle Management                 								*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::invalidate
 *
 *      Invalidate display
 */

void GraphicDisplay::invalidate()
{
    dirty.origin = { 0, 0 };
    dirty.size = size;
}

/*  GraphicDisplay::validate
 *
 *      Validate display
 */

void GraphicDisplay::validate()
{
    dirty = { 0, 0, 0, 0 };
}

/*	GraphicDisplay::markDirty
 *
 *		Mark region dirty. Internal routine used to update the dirty rectangle
 *  area.
 */

void GraphicDisplay::markDirty(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
	if ((width == 0) || (height == 0)) return;
	
	if ((dirty.size.width == 0) || (dirty.size.height == 0)) {
		dirty.size.width = width;
		dirty.size.height = height;
		dirty.origin.x = left;
		dirty.origin.y = top;
	} else {
		uint8_t right = left + width;
		uint8_t bottom = top + height;
		uint8_t dright = dirty.origin.x + dirty.size.width;
		uint8_t dbottom = dirty.origin.y + dirty.size.height;
		
		if (dirty.origin.x > left) dirty.origin.x = left;
		if (dirty.origin.y > top) dirty.origin.y = top;
		if (dright < right) dirty.size.width = right - dirty.origin.x;
		if (dbottom < bottom) dirty.size.height = bottom - dirty.origin.y;
	}
}


/****************************************************************************/
/*																			*/
/*	Font Manipulation                                       				*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::width
 *
 *      Width of character
 */

uint8_t GraphicDisplay::width(uint16_t c)
{
	if (font == NULL) return 0;		/* No font */

    if (c < font->first) return 0;
    if (c > font->last) return 0;

    return (font->glyph + (c - font->first))->xAdvance;
}

/*  GraphicDisplay::width
 *
 *      Width of string
 */

uint8_t GraphicDisplay::width(const char *str)
{
	char c;
	uint8_t len = 0;
	
	while (0 != (c = *str++)) len += width(c);
	return len;
}

/****************************************************************************/
/*																			*/
/*	Basic Drawing Routines  												*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::setPixel
 * 
 *      Set pixel. the public facing part which handles updating the invalid
 *  rectangle area
 */

void GraphicDisplay::setPixel(uint8_t x, uint8_t y)
{
	setPixelInternal(x,y);
	markDirty(x,y,1,1);
}


/****************************************************************************/
/*																			*/
/*	Character Drawing                                       				*/
/*																			*/
/****************************************************************************/

/*  GraphicDisplay::drawChar
 *
 *      Draw a pixel.
 */

void GraphicDisplay::drawChar(uint16_t c)
{
	const GFXglyph *gdata;
	uint16_t blen;
	uint8_t x,y,xo,yo;
	
	/*
	 *	Get the data for the glyph
	 */
    
	if (font == NULL) return;		/* No font */
	
    if (c < font->first) return;
    if (c > font->last) return;
    
    gdata = font->glyph + (c - font->first);

	/*
	 *	Get the dimensions, pixel location
	 */
	
	blen = ((uint16_t)gdata->width) * ((uint16_t)gdata->height);
	uint16_t bytes = (blen + 7)/8;
	
	/*
	 *	Calculate the start pixel position
	 */
	
	xo = pos.x + gdata->xOffset;
	yo = pos.y + gdata->yOffset;
	markDirty(xo,yo,gdata->width,gdata->height);	/* Mark drawing area dirty */
	
	/*
	 *	Run the bits
	 */
	
    uint16_t bitIndex = 0;
	for (y = 0; y < gdata->height; ++y) {
		for (x = 0; x < gdata->width; ++x) {
			uint8_t b = 0x80 >> (bitIndex & 7);
			uint16_t n = (bitIndex >> 3) + gdata->bitmapOffset;
			if (font->bitmap[n] & b) {
				setPixelInternal(x+xo,y+yo);
			}
            
            ++bitIndex;
		}
	}
	
	/*
	 *	Advance the cursor
	 */

	pos.x += gdata->xAdvance;
}

/*	GraphicDisplay::drawString
 *
 *		Draw the string
 */

void GraphicDisplay::drawString(const char *str)
{
	char c;
	
	while (0 != (c = *str++)) drawChar(c);
}

/****************************************************************************/
/*																			*/
/*	Graphic Drawing                                            				*/
/*																			*/
/****************************************************************************/

/*	GraphicDisplay::lineTo
 *
 *		Draw line to location
 */

void GraphicDisplay::lineTo(GDPoint pt)
{
	int16_t err,err2;
	int8_t sx,sy;
	uint8_t dx,dy;
	uint8_t minx,miny,maxx,maxy;
	
	/*
	 *	Drop into faster horizontal/vertical drawing routines if we can
	 */

	if (pt.x == pos.x) {
		if (pt.y > pos.y) {
			miny = pos.y;
			maxy = pt.y;
		} else {
			miny = pt.y;
			maxy = pos.y;
		}
		setVBarInternal(pt.x,miny,maxy);
		markDirty(pt.x,miny,1,maxy-miny+1);
        pos = pt;
		return;
	} 
	if (pt.y == pos.y) {
		if (pt.x > pos.x) {
			minx = pos.x;
			maxx = pt.x;
		} else {
			minx = pt.x;
			maxx = pos.x;
		}
		setHBarInternal(minx,maxx,pt.y);
		markDirty(minx,pt.y,maxx-minx+1,1);
        pos = pt;
		return;
	}
    
    /*
     *  Mark dirty for line
     */
    
	if (pt.x > pos.x) {
		minx = pos.x;
		maxx = pt.x;
	} else {
		minx = pt.x;
		maxx = pos.x;
	}
	if (pt.y > pos.y) {
		miny = pos.y;
		maxy = pt.y;
	} else {
		miny = pt.y;
		maxy = pos.y;
	}
	markDirty(minx,miny,maxx-minx+1,maxy-miny+1);

	/*
	 *	Set up the delta and stepping
	 */
	
	if (pt.x > pos.x) {
		dx = pt.x - pos.x;
		sx = 1;
	} else {
		dx = pos.x - pt.x;
		sx = -1;
	}
	
	if (pt.y > pos.y) {
		dy = pt.y - pos.y;
		sy = 1;
	} else {
		dy = pos.y - pt.y;
		sy = -1;
	}
	
	err = (int16_t)dx - (int16_t)dy;
	
    GDPoint p = pos;
	for (;;) {
		setPixelInternal(p.x,p.y);
		
		err2 = err << 1;
		if (err2 >= -(int16_t)dy) {
            if (p.x == pt.x) break;
			err -= dy;
			p.x += sx;
		}
		if (err2 <= dx) {
            if (p.y == pt.y) break;
			err += dx;
			p.y += sy;
		}
	}
    
    pos = pt;
}

/*	GraphicDisplay::paintRect
 *
 *		Paint rectangle
 */

void GraphicDisplay::paintRect(GDRect r)
{
	uint8_t right = r.origin.x + r.size.width;
	uint8_t bottom = r.origin.y + r.size.height - 1;	/* Because setVBarInternal includes bottom */
	for (uint8_t x = r.origin.x; x < right; ++x) {
		setVBarInternal(x,r.origin.y,bottom);
	}
	
	markDirty(r.origin.x,r.origin.y,r.size.width,r.size.height);
}

/*	GraphicDisplay::frameRect
 *
 *		Frame rectangle. Note this draws the rectangle which is interior to the
 *	provided rectangle boundaries.
 */

void GraphicDisplay::frameRect(GDRect r)
{
	if ((r.size.width == 0) || (r.size.height == 0)) return;
	markDirty(r.origin.x,r.origin.y,r.size.width,r.size.height);
	
	if (r.size.width == 1) {
		setVBarInternal(r.origin.x,r.origin.y,r.origin.y + r.size.height - 1);		
	} else if (r.size.height == 1) {
		setHBarInternal(r.origin.x,r.origin.x + r.size.width - 1,r.origin.y);
	} else {
		
		/*
		 *	Draw two vertical bars then fill horizontal
		 */
		
		uint8_t bottom = r.origin.y + r.size.height - 1;
		uint8_t right = r.origin.x + r.size.width;
		setVBarInternal(r.origin.x,r.origin.y,bottom);
		setVBarInternal(right - 1,r.origin.y,bottom);
		
		if (r.size.width > 2) {
			setHBarInternal(r.origin.x+1,right-2,r.origin.y);
			setHBarInternal(r.origin.x+1,right-2,bottom);
		}
	}
}

/*  GraphicDisplay::frameOval
 *
 *      Draw an oval
 */

void GraphicDisplay::frameOval(GDRect r)
{
    if ((r.size.width == 0) || (r.size.height == 0)) return;

    /* Mark dirty */
    markDirty(r.origin.x,r.origin.y,r.size.width,r.size.height);
    
    /* Preflight */
    uint8_t x0 = r.origin.x;
    uint8_t y0 = r.origin.y;
    uint8_t x1 = x0 + r.size.width;
    uint8_t y1 = y0 + r.size.height;
    
    int32_t a = r.size.width;
    int32_t b = r.size.height;
    int32_t b1 = b & 1;
    
    int32_t dx = 4 * (1 - a) * b * b;
    int32_t dy = 4 * (b1 + 1) * a * a;
    int32_t err = dx + dy + b1 * a * a;
    int32_t e2;
    
    y0 += (b + 1)/2;
    y1 = y0 - b1;
    a *= 8 * a;
    b1 = 8 * b * b;
    
    do {
        setPixelInternal(x1,y0);
        setPixelInternal(x0,y0);
        setPixelInternal(x0,y1);
        setPixelInternal(x1,y1);
        
        e2 = 2 * err;
        if (e2 <= dy) {
            y0++;
            y1--;
            dy += a;
            err += dy; 
        }
        if ((e2 >= dx) || (2 * err > dy)) {
            x0++;
            x1--;
            dx += b1;
            err += dx;
        }
    } while (x0 <= x1);
    
    while (y0 - y1 <= b) {
        setPixelInternal(x0-1,y0);
        setPixelInternal(x1+1,y0);
        ++y0;
        setPixelInternal(x0-1,y1);
        setPixelInternal(x1+1,y1);
        --y1;
    }
}


/*  GraphicDisplay::paintOval
 *
 *      Paint an oval
 */

void GraphicDisplay::paintOval(GDRect r)
{
    if ((r.size.width == 0) || (r.size.height == 0)) return;

    /* Mark dirty */
    markDirty(r.origin.x,r.origin.y,r.size.width,r.size.height);
    
    /* Preflight */
    uint8_t x0 = r.origin.x;
    uint8_t y0 = r.origin.y;
    uint8_t x1 = x0 + r.size.width;
    uint8_t y1 = y0 + r.size.height;
    
    int32_t a = r.size.width;
    int32_t b = r.size.height;
    int32_t b1 = b & 1;
    
    int32_t dx = 4 * (1 - a) * b * b;
    int32_t dy = 4 * (b1 + 1) * a * a;
    int32_t err = dx + dy + b1 * a * a;
    int32_t e2;
    
    y0 += (b + 1)/2;
    y1 = y0 - b1;
    a *= 8 * a;
    b1 = 8 * b * b;
    
    do {
        setVBarInternal(x0,y1,y0);
        setVBarInternal(x1,y1,y0);
        
        e2 = 2 * err;
        if (e2 <= dy) {
            y0++;
            y1--;
            dy += a;
            err += dy; 
        }
        if ((e2 >= dx) || (2 * err > dy)) {
            x0++;
            x1--;
            dx += b1;
            err += dx;
        }
    } while (x0 <= x1);
    
    while (y0 - y1 <= b) {
        setVBarInternal(x0-1,y1,y0);
        setVBarInternal(x1+1,y1,y0);
        ++y0;
        --y1;
    }
}

/*  GraphicDisplay::drawCorner
 *
 *      Draw or fill rounded corner, used for rounded rectangles
 */

void GraphicDisplay::drawCorner(uint8_t xm, uint8_t ym, uint8_t r, uint8_t cmask)
{
    int8_t x = -r;
    int8_t y = 0;
    int8_t err = 2 - 2 * r;
    bool flag = false;
    
    do {
        if (cmask & 0x10) {
            if (cmask & 1) setVBarInternal(xm-x,ym,ym+y);
            if (cmask & 2) setVBarInternal(xm-y,ym,ym-x);
            if (cmask & 4) setVBarInternal(xm+x,ym-y,ym);
            if (cmask & 8) setVBarInternal(xm+y,ym+x,ym);
        } else {
            if (cmask & 1) setPixelInternal(xm-x,ym+y);
            if (cmask & 2) setPixelInternal(xm-y,ym-x);
            if (cmask & 4) setPixelInternal(xm+x,ym-y);
            if (cmask & 8) setPixelInternal(xm+y,ym+x);
        }
        
        int8_t tmp = err;
        if (tmp <= y) {
            ++y;
            err += y*2+1;
        }
        if ((tmp > x) || (err > y)) {
            ++x;
            err += x*2+1;
        }
    } while (x <= 0);
}

/*  GraphicDisplay::frameRoundRect
 * 
 *      Frame the rounded rectangle
 */

void GraphicDisplay::frameRoundRect(GDRect r, uint8_t corner)
{
    uint8_t maxr = r.size.width;
    if (maxr > r.size.height) maxr = r.size.height;
    maxr >>= 1;
    
    if (corner > maxr) corner = maxr;
    
    /*
     *  Draw the rounded rectangle
     */
    
    drawCorner(r.origin.x + corner, r.origin.y + corner, corner, 0x04);
    setHBarInternal(r.origin.x + corner + 1, r.origin.x + r.size.width - corner - 2, r.origin.y);
    drawCorner(r.origin.x + r.size.width - corner - 1, r.origin.y + corner, corner, 0x08);
    setVBarInternal(r.origin.x + r.size.width - 1, r.origin.y + corner + 1, r.origin.y + r.size.height - corner - 2);
    drawCorner(r.origin.x + r.size.width - corner - 1, r.origin.y + r.size.height - corner - 1, corner, 0x01);
    setHBarInternal(r.origin.x + corner + 1, r.origin.x + r.size.width - corner - 2, r.origin.y + r.size.height - 1);
    drawCorner(r.origin.x + corner, r.origin.y + r.size.height - corner - 1, corner, 0x02);
    setVBarInternal(r.origin.x, r.origin.y + corner + 1, r.origin.y + r.size.height - corner - 2);
}

/*  GraphicDisplay::paintRoundRect
 * 
 *      Paint the rounded rectangle
 */

void GraphicDisplay::paintRoundRect(GDRect r, uint8_t corner)
{
    uint8_t maxr = r.size.width;
    if (maxr > r.size.height) maxr = r.size.height;
    maxr >>= 1;
    
    if (corner > maxr) corner = maxr;
    
    /*
     *  Draw the rounded rectangle
     */
    
    drawCorner(r.origin.x + corner, r.origin.y + corner, corner, 0x14);
    drawCorner(r.origin.x + r.size.width - corner - 1, r.origin.y + corner, corner, 0x18);
    drawCorner(r.origin.x + r.size.width - corner - 1, r.origin.y + r.size.height - corner - 1, corner, 0x11);
    drawCorner(r.origin.x + corner, r.origin.y + r.size.height - corner - 1, corner, 0x12);

    paintRect({r.origin.x,
            (uint8_t)(r.origin.y + corner + 1), 
            r.size.width, 
            (uint8_t)(r.size.height - corner * 2 - 2)});
    paintRect({(uint8_t)(r.origin.x + corner + 1),
            r.origin.y,
            (uint8_t)(r.size.width - corner * 2 - 2), 
            (uint8_t)(corner+1)});
    paintRect({(uint8_t)(r.origin.x + corner + 1),
            (uint8_t)(r.origin.y + r.size.height - corner - 1),
            (uint8_t)(r.size.width - corner * 2 - 2), 
            (uint8_t)(corner + 1)});
}