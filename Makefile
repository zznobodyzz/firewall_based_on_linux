ifeq ($(KERNELRELEASE), )
$(info "1st")
KDIR:=/lib/modules/$(shell uname -r)/build
all:
	gcc -o net_fire_wall net_fire_wall.c -lsqlite3
	#make -C $(KDIR) dtbs
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	rm *.ko *.o *.mod.c *.order *.symvers .*.cmd
else
$(info "2nd")
obj-m:=$(TAR).o
endif 
