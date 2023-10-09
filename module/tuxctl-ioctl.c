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

/* Global variables used to keep track of LED state and button state */
unsigned char leds;
unsigned char buttons;

/* Ack flag used to protect LEDs*/
int ack_flag;

/* Ioctl function declarations. */
int tux_init_ioctl(struct tty_struct* tty);
int tux_set_led_ioctl(struct tty_struct* tty, unsigned long arg);
int tux_buttons_ioctl(struct tty_struct* tty, unsigned long * arg);

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

	switch(a) {
		case MTCP_BIOC_EVENT:
			/* buttons is in this order: RIGHT|LEFT|DOWN|UP|C|B|A|START. Used logic to get proper buttons variable */
			buttons = (b & FOUR_BIT_MASK) | ((c & GET_RIGHT_AND_UP) << SHIFT_RIGHT_AND_UP) | ((c & GET_LEFT) << SHIFT_LEFT) | ((c & GET_DOWN) << SHIFT_DOWN);
			break;
		case MTCP_ACK:
			ack_flag = 1; // reset ack_flag back to 1 after command is completed
			break;
		case MTCP_RESET:
			/* Return Tux to usable state */
			tux_set_led_ioctl(tty, leds);
			tux_init_ioctl(tty);
			break;
		default:
			break;
	}

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

/* 
 * tuxctl_ioctl
 *   DESCRIPTION: Main ioctl which checks the input command, and calls
 * 				  the proper helper function
 *   INPUTS: tty, file, cmd, arg
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful, -EINVAL if failed
 *   SIDE EFFECTS: Executes the proper command.
 */
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tux_init_ioctl(tty);
	case TUX_BUTTONS:
		return tux_buttons_ioctl(tty, (unsigned long *) arg);
	case TUX_SET_LED:
		return tux_set_led_ioctl(tty, arg);
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

/* 
 * tux_init_ioctl
 *   DESCRIPTION: Initializes any variables associated with the driver and returns 0.
 *   INPUTS: tty
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: Enable Button interrupt-on-change. Put the LED display into user-mode.
 */
int tux_init_ioctl(struct tty_struct* tty) {
	int num_bytes = 2; // 2 bytes for MTCP_BIOC_ON and MTCP_LED_USR
	unsigned char buf[num_bytes];
	buf[0] = MTCP_BIOC_ON;
	buf[1] = MTCP_LED_USR;
	ack_flag = 0; // initialize ack_flag to 0
	tuxctl_ldisc_put(tty, buf, num_bytes);
	return 0;
}

/*	The hex values corresponding to the 7 segment display of each hex value from 0 to F.	*/
unsigned char led_segments[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAE, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

/* 
 * tux_set_led_ioctl
 *   DESCRIPTION: Takes a 32-bit integer as argument. 
 * 				  Creates buffer for LEDs according to the input.
 *   INPUTS: tty, arg
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: Creates a buffer for LEDs according to the input.
 * 				   Updates global LED variable.
 */
int tux_set_led_ioctl(struct tty_struct* tty, unsigned long arg) {
	unsigned int num_bytes = 2; // two bytes for opcode and LED states
	unsigned int led_on, decimal_points, cur_led, cur_segment; // variables to help with this function
	int i; // loop counter
	unsigned char buf[NUM_LEDS + num_bytes];
	
	/* Check ack_flag */
	if (!ack_flag) {
		/* If ack_flag is 0, do not proceed */
		return 0;
	}
	ack_flag = 0; // set ack_flag to 0 before proceeding
	led_on = (arg >> ARG_SHIFT) & FOUR_BIT_MASK;
	/* Update the LED global variable */
	leds = arg;

	buf[0] = MTCP_LED_SET;	// opcode
	buf[1] = 0x0F;

	/* decimal point information */
	decimal_points = arg >> DECIMAL_SHIFT;

	for (i = 0; i < NUM_LEDS; i++) {
		cur_segment = 0x00; // 0x00 initializes to blank (nothing is on)
		if (led_on & GET_LSB) {	// this means the LED is on
			cur_led = FOUR_BIT_MASK & arg; // get current LED info
			cur_segment = led_segments[cur_led]; // get corresponding 7-segment display
			cur_segment |= ((decimal_points & GET_LSB) << NUM_BITS_LED); // add decimal point if necessary
		}
		else {
			cur_segment |= ((decimal_points & GET_LSB) << NUM_BITS_LED); // add decimal point if necessary
		}
		buf[i + num_bytes] = cur_segment;
		/* Go to next LED */
		led_on >>= LED_NEXT;
		arg >>= ARG_NEXT;
		decimal_points >>= DECIMAL_NEXT;
	}
	tuxctl_ldisc_put(tty, buf, NUM_LEDS + num_bytes);
	return 0;
}

/* 
 * tux_buttons_ioctl
 *   DESCRIPTION: Takes pointer to 32-bit integer. Copy the buttons
 * 				  global variable to the user
 *   INPUTS: tty, arg
 *   OUTPUTS: -EINVAL if pointer is invalid of failed copying, otherwise 0
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: Copy buttons variable to user, or -EINVAL if failed
 */
int tux_buttons_ioctl(struct tty_struct* tty, unsigned long * arg) {
	if (arg == NULL) {
		return -EINVAL;
	}

	/* copy buttons variable to user, return -EINVAL if there are bytes that failed to be copied */
	if (copy_to_user(arg, &buttons, 1) > 0) {
		return -EINVAL;
	}


	return 0;
}
