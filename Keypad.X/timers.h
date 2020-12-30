/*  timers.h
 *
 *      Set up the timers for periodic delays
 */

#ifndef _TIMERS_H    /* Guard against multiple inclusion */
#define _TIMERS_H

#include <stdint.h>

#define SYSFREQ 48000000            /* 60MHz */

#ifdef __cplusplus
extern "C" {
#endif

extern void     InitMillisecondTimer();
extern void     ShutdownMillisecondTimer();
extern uint32_t GetMilliseconds(void);
extern void     DelayMilliseconds(uint16_t ms);
    

#ifdef __cplusplus
}
#endif

#endif /* _TIMERS_H */
