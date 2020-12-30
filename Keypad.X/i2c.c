/*  i2c.c
 *  
 *      Two wire communication library. Note this uses the I2C 1 module on
 *  the PIC32MX270F256B, and may not be appropriate for other PIC32 processors.
 */

#include <xc.h>
#include <sys/attribs.h>
#include "timers.h"
#include "i2c.h"

/****************************************************************************/
/*																			*/
/*	Constants       														*/
/*																			*/
/****************************************************************************/

/*  PERIPHERAL_CLOCK
 * 
 *      Indicates the peripheral clock rate. For this example, our settings
 *  create a peripheral clock rate of 6mhz. This value must be set by hand
 *  when recompiling this code for your target platform.
 */
#define PERIPHERAL_CLOCK        6000000     /* 6mhz, from other settings */

/*  TWIState.state values
 *
 *      We need to track what our software is up to in order to interpret
 *  interrupts correctly. Note this is the states for a master, not a slave.
 * 
 *      When writing data we pass through the following states:
 *
 *  1.  Idle state.
 * 
 *  2.  Starting. The start condition or repeated start condition was sent, and
 *      we're waiting for the start condition to complete. On completion we will
 *      send the address of the device we're writing to.
 * 
 *  3.  Address. The address was just sent, and we're waiting for a response
 *      for ack/nak to the address. This will trip the start of reading or
 *      writing: if reading, we set up to read the next byte. To write, we
 *      actually write the next byte.
 * 
 *  4.  Write. This state arrives to determine if we got an ACK or a NAK, and
 *      either sends the next byte or sends a stop (or repeat stop) command.
 * 
 *  5.  Read. This is called after we've requested a read, and stores the 
 *      byte (or deals with the stop condition). If we read a byte, we then
 *      trigger the ACK/NAK response depending on if we can read more data.
 *      This also handles the stop command.
 * 
 *  6.  Stop. This state occurs after we've sent a stop command. This happens
 *      for multiple reasons. This transitions to the idle state once the
 *      stop command (or repeated start) is processed. 
 */

#define TWISTATE_IDLE           0
#define TWISTATE_STARTING       1       /* Start or repeated start condition */
#define TWISTATE_ADDRESS        2       /* Address byte is written */
#define TWISTATE_READ           3       /* Reading data */
#define TWISTATE_READACK        4       /* Read send ACK to continue reading */
#define TWISTATE_READNAK        5       /* Send NAK at end of read for stop */
#define TWISTATE_WRITE          6       /* Writing data */
#define TWISTATE_STOP           7       /* Stopping */

/****************************************************************************/
/*																			*/
/*	Globals            														*/
/*																			*/
/****************************************************************************/

/*  TWIState
 *
 *      Stores the state and state flags for our I2C module
 */
static volatile struct {
    uint8_t address;            // I2C 7 bit address + R/W byte to send
    
    uint8_t sendStop:1;         // Set to send stop flag at EOT
    uint8_t inRepeatStart:1;    // If sendStop false, set to true at EOT for repeat start
    uint8_t state:3;            // What am I doing state?
    uint8_t error:3;            // Error code if there was a problem
} TWIState;

static volatile uint8_t *TWIMasterBuffer;        /* Buffer being read or written */
static volatile uint8_t TWIMasterBufferLength;   /* Length of buffer */
static volatile uint8_t TWIMasterBufferIndex;    /* Read/write position of buffer */

/****************************************************************************/
/*																			*/
/*	Startup/Shutdown														*/
/*																			*/
/****************************************************************************/

/*  TWIInit
 *
 *      Enable I2C (two wire) interface
 */

void TWIInit(uint32_t frequency)
{
    /*
     *  Initialize the I2C 1 module
     */
    I2C1CON = 0;                /* Turn off I2C1 module */
    I2C1CONbits.DISSLW = 1;     /* Disable slew rate for 100k */
    I2C1CONbits.SMEN = 0;       /* I2C voltage standards */
    I2C1CONbits.SIDL = 0;       /* Continue on idle */
    
    
    double BRG = frequency;
    BRG = (1 / (2 * BRG)) - 0.000000104;
    BRG *= (PERIPHERAL_CLOCK) - 2;
    
    I2C1BRG = (int)BRG;         /* Set clock speed */
    
    /*
     *  Initialize internal state
     */
    
    TWIState.sendStop = 0;
    TWIState.inRepeatStart = 0;
    TWIState.error = 0;
    TWIState.address = 0;
    TWIMasterBuffer = NULL;
    
    /*
     *  Initialize interrupts
     */
    
    IFS1bits.I2C1BIF = 0;
    IFS1bits.I2C1MIF = 0;
    IFS1bits.I2C1SIF = 0;       /* Clear interrupt flag for I2C 1 module */
    
    IPC8bits.I2C1IP = 6;        /* Interrupt priority 6 -- fairly time critical */
    IPC8bits.I2C1IS = 3;        /* Subpriority 3 */
    IEC1bits.I2C1MIE = 1;       /* Enable master interrupt */
    IEC1bits.I2C1BIE = 1;       /* Interrupt bus control interrupt */
    IEC1bits.I2C1SIE = 0;       /* DO NOT enable slave interrupt control. */
    
    I2C1CONbits.ON = 1;         /* Turn on I2C1 module */
}

/*  TWIShutdown
 *
 *      Deactivates the I2C1 module and disable interrupts
 */
void TWIShutdown(void)
{
    I2C1CON = 0;                /* Turn off I2C1 module */

    IFS1bits.I2C1BIF = 0;
    IFS1bits.I2C1MIF = 0;
    IFS1bits.I2C1SIF = 0;       /* Clear interrupt flag for I2C 1 module */

    IEC1bits.I2C1MIE = 0;       /* Enable master interrupt */
    IEC1bits.I2C1BIE = 0;       /* Interrupt bus control interrupt */
    IEC1bits.I2C1SIE = 0;       /* DO NOT enable slave interrupt control. */
}

// I2C_wait_for_idle() waits until the I2C peripheral is no longer doing anything  
void TWIWaitIdle(void)
{
    while(I2C1CON & 0x1F);      // Acknowledge sequence not in progress
                                // Receive sequence not in progress
                                // Stop condition not in progress
                                // Repeated Start condition not in progress
                                // Start condition not in progress
    while(I2C1STATbits.TRSTAT); // Bit = 0 ? Master transmit is not in progress
}

/**
 * Performs a write operation. Note this is synchronous.
 * @param addr
 * @param data
 * @param len
 * @return 
 */
int8_t TWIWrite(uint8_t addr, const uint8_t *data, uint8_t len, bool stop)
{
    /*
     *  Wait until idle. 
     */
    
    while (TWIState.state != TWISTATE_IDLE) ;   /* Wait until we're in idle state */
    TWIWaitIdle();                              /* Also verify hardware idle */
    
    /*
     *  Now set up to send
     */
    
    TWIState.address = addr << 1;               /* MSB == 0: write */
    TWIState.sendStop = stop;
    TWIState.error = 0;
    TWIMasterBuffer = (uint8_t *)data;
    TWIMasterBufferLength = len;
    TWIMasterBufferIndex = 0;
    
    if (TWIState.inRepeatStart) {
        /* In repeat start. Repeat start already sent, so send address */
        TWIState.state = TWISTATE_ADDRESS;
        I2C1TRN = TWIState.address;
    } else {
        /* Need to send start. */
        TWIState.state = TWISTATE_STARTING;
        I2C1CONbits.SEN = 1;
    }
    
    /*
     *  Now wait until done
     */
    
    while (TWIState.state != TWISTATE_IDLE) ;
    
    /*
     *  Return results
     */
    
    if (TWIState.error != TWI_SUCCESS) {
        return - TWIState.error;
    } else {
        return TWIMasterBufferLength;
    }
}

/**
 * Performs a read operation. Note this is synchronous.
 * @param addr
 * @param data
 * @param len
 * @return 
 */
int8_t TWIRead(uint8_t addr, uint8_t *data, uint8_t len, bool stop)
{
    /*
     *  Wait until idle. 
     */
    
    while (TWIState.state != TWISTATE_IDLE) ;   /* Wait until we're in idle state */
    TWIWaitIdle();                              /* Also verify hardware idle */
    
    /*
     *  Now set up to send
     */
    
    TWIState.address = 0x01 | (addr << 1);      /* MSB == 1: read */
    TWIState.sendStop = stop;
    TWIState.error = 0;
    TWIMasterBuffer = data;
    TWIMasterBufferLength = len;
    TWIMasterBufferIndex = 0;
    
    if (TWIState.inRepeatStart) {
        /* In repeat start. Repeat start already sent, so send address */
        TWIState.state = TWISTATE_ADDRESS;
        I2C1TRN = TWIState.address;
    } else {
        /* Need to send start. */
        TWIState.state = TWISTATE_STARTING;
        I2C1CONbits.SEN = 1;
    }
    
    /*
     *  Now wait until done
     */
    
    while (TWIState.state != TWISTATE_IDLE) ;
    
    /*
     *  Return results
     */
    
    if (TWIState.error != TWI_SUCCESS) {
        return - TWIState.error;
    } else {
        return TWIMasterBufferLength;
    }
}

/****************************************************************************/
/*																			*/
/*	Interrupts      														*/
/*																			*/
/****************************************************************************/

/*
 *  IPC1 interrupt handler. 
 * 
 *      Note: to start sending or receiving we set the global variables, then
 *  we start off the process by sending the start signal, or if we are in a
 *  repeated start state, we send the address directly and move into the
 *  address state.
 */
void __ISR(_I2C_1_VECTOR, IPL6AUTO) IPC1Handler(void)
{
    /*
     *  Determine why we were woken up. There are three interrupt flags we
     *  could receive.
     */
    
    if (IFS1bits.I2C1SIF) {
        /* Clear slave bit. We're not a slave so we just return */
        IFS1CLR = _IFS1_I2C1SIF_MASK;
//      IFS1bits.I2C1SIF = 0;
        return;
    }
    
    if (IFS1bits.I2C1BIF) {
        /*  24.4.2.3
         * 
         *  Bus collision interrupts. This happens if there is a collision
         *  (i.e., multiple masters or bus error) if any of the following
         *  things are attempted during master transmission:
         * 
         *  - Start condition
         *  - Repeated Start condition
         *  - Address bit
         *  - Data bit
         *  - Acknowledge bit
         *  - Stop condition
         * 
         *  For our code, ideally we would restart the transmission event,
         *  but for now we simply set an error condition and punt.
         */
        
        IFS1CLR = _IFS1_I2C1BIF_MASK;
//      IFS1bits.I2C1BIF = 0;
        
        TWIState.state = TWISTATE_STOP;
        TWIState.error = -TWI_ERROR_BUS;
        I2C1CONbits.PEN = 1;            /* Set stop condition immediately */
        return;
    }
    
    if (IFS1bits.I2C1MIF) {
        /*  24.4.2.1
         * 
         *  Master mode operations. This happens when one of the following events
         *  take place that require us to perform an action. Those actions are:
         * 
         *  - Start condition
         *  - Repeated start condition
         *  - Stop condition
         *  - Data transfer byte received from slave
         *  - ACK/NAK condition transmitted to slave
         *  - Data transfer from master
         *  - Stop detected
         */
        
        IFS1CLR = _IFS1_I2C1MIF_MASK;
//      IFS1bits.I2C1MIF = 0;
        
        /*
         *  Determine why this was called and take the next appropriate
         *  action.
         */
        
        if (TWIState.state == TWISTATE_STARTING) {
            /*
             *  We're waiting for both the send and repeat send conditions to
             *  complete.
             */
            
            if (!I2C1CONbits.SEN && !I2C1CONbits.RSEN) {
                /*
                 *  Write the address. Assumption: the interrupt occurred as
                 *  we were finished writing the start/repeat start state. We
                 *  also assume our interrupt routine is the only thing allowed
                 *  to touch our I2C1 registers, so we can safely assume at
                 *  this point our hardware is in an idle state.
                 * 
                 *  Note we transition to the address state, which always
                 *  expects a ACK/NAK response but then dispatches to the
                 *  appropriate read/write state.
                 */
                
                TWIMasterBufferIndex = 0;
                TWIState.state = TWISTATE_ADDRESS;
                I2C1TRN = TWIState.address;
            } else {
                /*
                 *  This shouldn't happen.
                 */
                
                TWIState.state = TWISTATE_STOP;    /* We went south here. */
                TWIState.inRepeatStart = 0;
                TWIState.error = -TWI_ERROR_INTERNAL;
                I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
            }
        } else if (TWIState.state == TWISTATE_ADDRESS) {
            /*
             *  We've written the address. This processes the ACK or NAK state
             *  received. If we receive a NAK, process as an address error.
             *  Otherwise this triggers the appropriate thing to either read
             *  or write a thing.
             */

            if (!I2C1STATbits.TRSTAT) {
                if (I2C1STATbits.ACKSTAT) {
                    /*
                     *  NAK on address
                     */
                    
                    TWIState.state = TWISTATE_STOP;
                    TWIState.error = -TWI_ERROR_WRITE_ADDRESS;
                    
                    TWIState.inRepeatStart = 0;
                    I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
                } else {
                    /*
                     *  ACK. Determine what to do next.
                     */
                    
                    if (TWIMasterBufferLength == 0) {
                        /*
                         *  We can't read or write, so simply send stop.
                         */
                       
                        TWIState.state = TWISTATE_STOP;
                        TWIState.inRepeatStart = 0;
                        I2C1CONbits.PEN = 1;            /* Set stop condition immediately */
                    } else if (TWIState.address & 1) {
                        /*
                         *  Read. Set up to read the first byte
                         */
                        
                        I2C1CONbits.RCEN = 1;           /* Set up to read next byte */
                        TWIState.state = TWISTATE_READ; /* And note we're reading. */
                    } else {
                        /*
                         *  Write. Set up and write the first byte.
                         */
                        
                        TWIMasterBufferIndex = 0;
                        I2C1TRN = TWIMasterBuffer[TWIMasterBufferIndex++];
                        TWIState.state = TWISTATE_WRITE;    /* Just wrote; set up to handle */
                    }
                }
                
            } else {
                /* This shouldn't happen */
                TWIState.state = TWISTATE_STOP;    /* We went south here. */
                TWIState.inRepeatStart = 0;
                TWIState.error = -TWI_ERROR_INTERNAL;
                I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
            }
        } else if (TWIState.state == TWISTATE_WRITE) {
            /*
             *  This state arrives when we've written a byte and we're expecting
             *  either an ACK or a NAK. On NAK we stop; on ACK we send another or
             *  we stop if we're out.
             */
            
            if (!I2C1STATbits.TRSTAT) {
                if (I2C1STATbits.ACKSTAT) {
                    /* NAK received. */
                    TWIState.state = TWISTATE_STOP;
                    TWIState.inRepeatStart = 0;
                    TWIState.error = -TWI_ERROR_WRITE_DATA;
                    I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
                } else if (TWIMasterBufferIndex >= TWIMasterBufferLength) {
                    /* We're out of stuff to write, so just stop */
                    if (TWIState.sendStop) {
                        TWIState.inRepeatStart = 0;
                        I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
                    } else {
                        TWIState.inRepeatStart = 1;
                        I2C1CONbits.RSEN = 1;
                    }
                    TWIState.state = TWISTATE_STOP;
                } else {
                    /* ACK received. Write the next item */
                    I2C1TRN = TWIMasterBuffer[TWIMasterBufferIndex++];
                }
            } else {
                /* This shouldn't happen */
                TWIState.state = TWISTATE_STOP;    /* We went south here. */
                TWIState.inRepeatStart = 0;
                TWIState.error = -TWI_ERROR_INTERNAL;
                I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
            }
        } else if (TWIState.state == TWISTATE_READ) {
            /*
             *  This is called when we receive a byte. We stash the byte and
             *  take the appropriate action. If we receive a stop, we handle
             *  that separately.
             */
            
            if (I2C1STATbits.RBF) {
                TWIMasterBuffer[TWIMasterBufferIndex++] = I2C1RCV;
                
                if (TWIMasterBufferIndex >= TWIMasterBufferLength) {
                    /* At end, send NAK. */
                    I2C1CONbits.ACKDT = 1;  /* NAK */
                    TWIState.state = TWISTATE_READNAK;
                } else {
                    /* At end, send ACK */
                    I2C1CONbits.ACKDT = 0;  /* ACK */
                    TWIState.state = TWISTATE_READACK;
                }
                I2C1CONbits.ACKEN = 1;  /* Send NAK */
            } else {
                /* This shouldn't happen */
                TWIState.state = TWISTATE_STOP;    /* We went south here. */
                TWIState.inRepeatStart = 0;
                TWIState.error = -TWI_ERROR_INTERNAL;
                I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
            }
            
             
        } else if (TWIState.state == TWISTATE_READACK) {
            /*
             *  This happens after we've sent the ACK. The I2C protocol simply
             *  readies for the next byte to be read. Note the slave has no
             *  choice but continue sending bytes until the master sends a
             *  NAK.
             */
            
            I2C1CONbits.RCEN = 1;           /* Set up to read next byte */
            TWIState.state = TWISTATE_READ; /* And note we're reading. */
        } else if (TWIState.state == TWISTATE_READNAK) {
            /*
             *  We sent the NAK. Now send the stop condition (or repeated
             *  start condition
             */
            
            if (TWIState.sendStop) {
                I2C1CONbits.PEN = 1;                /* Set stop condition immediately */
                TWIState.inRepeatStart = 0;
            } else {
                I2C1CONbits.RSEN = 1;
                TWIState.inRepeatStart = 1;
            }
            TWIState.state = TWISTATE_STOP;
        } else if (TWIState.state == TWISTATE_STOP) {
            /*
             *  This is called when we're stopping. All we do here is move to
             *  the idle state.
             */
            
            TWIState.state = TWISTATE_IDLE;
        }
    }
}