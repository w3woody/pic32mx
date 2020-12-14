/*  timers.c
 *
 *      Set up a millisecond timer interrupt and allow for delays by waiting
 *  for elapsed time on the millsecond clock. This uses Timer 1.
 */

#include <stdint.h>
#include <xc.h>
#include <sys/attribs.h>
#include "timers.h"

/****************************************************************************/
/*																			*/
/*	Globals 																*/
/*																			*/
/****************************************************************************/

static volatile uint32_t  GMilliseconds;        /* Wraps every 497 days */

/****************************************************************************/
/*																			*/
/*	Interrupt																*/
/*																			*/
/****************************************************************************/

void InitMillisecondTimer()
{
    GMilliseconds = 0;  /* Reset millisecond counter */
    T1CON = 0;          /* Disable Timer 1 */
    
    TMR1 = 0;           /* Set timer 1 counter to 0 */
    IEC0bits.T1IE = 1;  /* Enable interrupt */
    
    /*
     *  Set the counter, scaler. This is fed from our peripheral clock,
     *  which is set at 1/8th the internal clock speed. (That is, the
     *  peripheral clock is at 6mhz)
     */
   
    T1CONbits.TCKPS = 0b01; /* 1/8 step */
    PR1 = 7500;             /* 100hz. (48mhz / 8 / 8 / 100) */
    
    /*
     *  Set up interrupt
     */
    
    IFS0bits.T1IF = 0;  /* Clear interrupt flag for timer 1 */
    IPC1bits.T1IP = 7;  /* Interrupt level 7 */
    IPC1bits.T1IS = 3;  /* Subpriority 3 */
    IEC0bits.T1IE = 1;  /* Enable interrupts */
    
    /*
     *  Enable multivector mode (if not already enabled) and
     *  initialize interupts
     */
    
    INTCONbits.MVEC = 1;    /* Multivector mode */
    __builtin_enable_interrupts();
    
    /*
     *  Turn on timer
     */
    
    T1CONbits.TON = 1;  /* Turn on interrupts */

}

uint32_t GetMilliseconds(void)
{
    uint32_t v;
    
    unsigned int state = __builtin_get_isr_state();
    __builtin_disable_interrupts();
    
    v = GMilliseconds;

    __builtin_set_isr_state(state);
    
    return v;
}

void ShutdownMillisecondTimer()
{
    T1CON = 0;          /* Disable timer 1 */ 
}

void DelayMilliseconds(uint16_t delay)
{
	uint32_t time = GetMilliseconds() + delay;
	while (time > GetMilliseconds()) {
    }
}

/*  Timer interrupt. At level 7 since this is a core function */
void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
    IFS0bits.T1IF = 0;
    GMilliseconds = GMilliseconds + 1;
}

