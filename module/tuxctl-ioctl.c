/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/*	The hex values corresponding to the 7 segment display of each hex value from 0 to F.	*/
unsigned char led_segments = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tux_init_ioctl(tty);
	case TUX_BUTTONS:
		return tux_buttons_ioctl(tty, arg);
	case TUX_SET_LED:
		return set_led_ioctl(tty, arg);
	case TUX_LED_ACK:
		return 0;
	case TUX_LED_REQUEST:
		return 0;
	case TUX_READ_LED:
		return 0;
	default:
	    return -EINVAL;
    }
}

/* Initializes any variables associated with the driver and returns 0. */
int tux_init_ioctl(struct tty_struct* tty) {
	return 0;
}

int set_led_ioctl(struct tty_struct* tty, unsigned long arg) {
	unsigned int num_bytes = 2;	// start with 2 bytes guaranteed, will be incremented based on number of LEDs that are on
	unsigned char buf[NUM_LEDS + num_bytes];
	unsigned int led_on = (arg >> 16) & 0x0F;
	buf[0] = MTCP_LED_SET;	// opcode
	buf[1] = led_on;	//	first byte is LEDs to be set

	unsigned int get_cur_led = 0x0F;	// bitwise & with this to get cur_led
	unsigned int decimal_points = arg >> 24;

	int i;	// loop counter
	for (i = 0; i < NUM_LEDS; i++) {
		if (led_on & 0x01) {	// this means the LED is on
			unsigned int cur_led = get_cur_led & arg;
			unsigned int cur_segment = led_segments[cur_led];
			cur_segment |= ((decimal_points & 0x01) << 4);
			buf[num_bytes] = cur_segment;
			num_bytes++;
		}
		led_on >>= 1;
		arg >>= 4;
		decimal_points >>= 1;
	}
	tuxctl_ldisc_put(tty, buf, num_bytes);
	return 0;
}

int tux_buttons_ioctl(struct tty_struct* tty, unsigned long arg) {
	if (arg == NULL) {
		return -EINVAL;
	}

	return 0;
}