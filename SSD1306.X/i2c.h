/*  i2c.h
 *
 *      Two wire 
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <stdbool.h>

/****************************************************************************/
/*																			*/
/*	Routines																*/
/*																			*/
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Frequency
 */

#define TWI_FREQ					100000L	/* Typical frequency */

/*
 *	Errors
 */

#define TWI_SUCCESS					0
#define TWI_ERROR_LENGTH_TOO_LONG	-1		/* Read/Write */
#define TWI_ERROR_WRITE_ADDRESS		-2		/* NAK on writing address */
#define TWI_ERROR_WRITE_DATA		-3		/* NAK on writing data */
#define TWI_ERROR_ARBITRATION		-4		/* Lost arbitration on multi-master environment */
#define TWI_ERROR_BUS				-5		/* Bus error */
#define TWI_ERROR_INTERNAL          -6      /* Internal state error */

/*
 *	Methods
 */

extern void TWIInit(uint32_t frequency);
extern void TWIShutdown(void);

/* Master; read and write are synchronous */
extern int8_t TWIWrite(uint8_t addr, const uint8_t *data, uint8_t len, bool stop);
extern int8_t TWIRead(uint8_t addr, uint8_t *data, uint8_t len, bool stop);

#ifdef __cplusplus
};
#endif

#endif /* __I2C_H__ */
