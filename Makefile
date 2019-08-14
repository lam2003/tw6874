export ARCH = arm
export AR = arm-hisiv400-linux-gcc
export CC = arm-hisiv400-linux-gcc
export EXTRA_CFLAGS += $(DEBFLAGS) -Wall
export EXTRA_CFLAGS += -I$(LDDINC)
export LD = arm-hisiv400-linux-ld
ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m := tw6874_driver_1.o tw6874_driver_2.o
tw6874_driver_1-objs := tw6874_1.o tw6874drv_1.o
tw6874_driver_2-objs := tw6874_2.o tw6874drv_2.o
else
KERNELDIR ?= /win/linux-3.0.y
PWD := $(shell pwd)
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules
	$(CC) test_tw6874.c -o test_tw6874
	cp ./*.ko /nfs/
	cp ./test_tw6874 /nfs/
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include clean
endif
