obj-m += io.o
obj-m += rd.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -g fp.c -o fp
	chmod 700 fp

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm fp
