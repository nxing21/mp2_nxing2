// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)

/* These variables used to help with logic in the ioctls. */
#define NUM_LEDS    4
#define NUM_BITS_LED    4
#define ARG_NEXT    4
#define DECIMAL_NEXT   1
#define LED_NEXT       1
#define GET_LSB         0x01
#define DECIMAL_SHIFT   24
#define ARG_SHIFT       16
#define FOUR_BIT_MASK   0x0F
#define GET_RIGHT_AND_UP    0x09
#define GET_LEFT    0x02
#define GET_DOWN    0x04
#define SHIFT_RIGHT_AND_UP  4
#define SHIFT_LEFT  5
#define SHIFT_DOWN  3

#endif

