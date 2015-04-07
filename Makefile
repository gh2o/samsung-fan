obj-m := samsung-wmi.o

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
