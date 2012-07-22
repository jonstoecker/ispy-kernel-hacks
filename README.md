ispy-kernel-hacks
=================

Linux kernel module and driver experimentation.

Overview
========
fp.c : this is a user space program that uniquely identifies a given hardware
configuration by marking the system's MAC address and primary hard disk serial.
Has a write mode (writes a fingerprint file) and a read file (using a -r flag)
that can check a fingerprint file against the currently running system.

io.c : this is a crude keylogger (taking the form of a kernel module) that 
intercepts keystroke calls and then outputs them to the kernel log and
the terminal window from where the module was installed.

rd.c : kernel-level execution hack. Can replace a given execution call with
another by searching every path string that runs through the execution
system call and replacing the appropriate one with another.
