/* RD.C ==================================================================== */
// @desc    Kernel module to redirect execution output. Intercepts function
//          calls to open_exec. Can be used with load time parameters or,
//			by default, corrects the heathen ways of errant emacs users. 
//
// @author  Jon Stoecker <jws527@gmail.com>
// @date    4/17/2012
//
// Tested with GCC 4.6.1 in Ubuntu Linux 11.10 / kernel version 3.0.0-17
/* ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/string.h>

/*
 *  Definitions 
 */
#define ASM "\x68\x00\x00\x00\x00\xc3" 
#define BYTES 6 
#define TEST

#define AUTHOR "Jonathan Stoecker <jon.stoecker@gmail.com>"
#define DESC "Redirects execution output."
#define VERSION "1.0"
#define LICENSE "GPL"

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
MODULE_VERSION(VERSION);
MODULE_LICENSE(LICENSE);

char* path = "/usr/bin/nano";
module_param(path, charp, 0000);
MODULE_PARM_DESC(path, "  Original/intended target.");
char* new_path = "/usr/bin/vi";
module_param(new_path, charp, 0000);
MODULE_PARM_DESC(new_path, "  Redirected target.");

char original[6];       // Original map as taken from the open_exec symbol
char new[6] = ASM;      // Modified map with jump instruction

/*
 *  String allocation functions
 */
int _strlen (char *a)
{
    int x = 0;
    while (*a != 0)
    {
        x++;
        *a++;
    }
    return x;
}
char *_strdup (char *a)
{
    char* x = NULL;
    int y = _strlen (a) + 1;
    int z;

    x = kmalloc (y, GFP_KERNEL);
    memset (x, 0, y);
    y--;

    for (z = 0; z < y; z++) {
        x[z] = a[z];
    }

    return x;
}

/*
 *  Redirect function: this function will handle calls to exec_open
 *  while the module is installed.
 */
struct file* redir_exec(const char* name)
{
    struct file* new_file;
    char* new_name = _strdup(new_path); 

#ifdef TEST 
    printk(KERN_INFO "Redirect: in open_exec\n");
    printk(KERN_INFO "Redirect: cmd: %s\n", name);
#endif

    // Restore original function to process exe path
    // as kernel expects; test for path to be redirected 
    memcpy(open_exec, original, BYTES);
    if (!strcmp(name, path)) {
        new_file = open_exec(new_name);
#ifdef TEST
        printk(KERN_INFO "Redirect: new cmd: %s\n", new_name);
#endif
    } else {
        new_file = open_exec(name);
    }
    memcpy(open_exec, new, BYTES);

#ifdef TEST   
    printk(KERN_INFO "Redirect: open_exec done\n");
#endif

    return new_file;
}

/*
 *  Module intialization function
 */
static int __init rd_init(void)
{
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8) )
    printk(KERN_WARNING "Redirect: error: designed for ker 2.6.8+\n");
	return -1;
#endif
   
    // Insert a jump command into the first six bits of the 
    // open_exec symbol's memory address to redirect to my function 
    *(unsigned long*) &new[1] = (unsigned long)redir_exec;

    // For Intel processors, disable protected mode (bit 0) in 
    // the CR0 control register
#ifdef CONFIG_X86
	write_cr0 (read_cr0() & (~0x10000));
#endif

    memcpy(original, open_exec, BYTES);
    memcpy(open_exec, new, BYTES);

	printk("Redirect: loaded\n");
    return 0;
}

/*
 *  Module cleanup function
 */
static void __exit rd_exit(void)
{
    // Restore the byte mapping for the original open_exec function
    // and reset the cr0 write protect bit
    memcpy(open_exec, original, BYTES);
    
    // For Intel processors, restore protected mode (bit 0) in 
    // the CR0 control register
#ifdef CONFIG_X86
	write_cr0 (read_cr0() | 0x10000);	
#endif

    printk("Redirect: unloaded.\n");
}

module_init(rd_init);
module_exit(rd_exit);

