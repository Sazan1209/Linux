obj-m += fifo_module.o

ABS_PATH_TO_VROOT ?= ../initramfs
PATH_TO_MODULE_BUILD=$(ABS_PATH_TO_VROOT)/lib/modules/6.7.4/build

all:
	make -C "$(PATH_TO_MODULE_BUILD)" M="$(PWD)" modules
clean:
	make -C "$(PATH_TO_MODULE_BUILD)" M="$(PWD)" clean