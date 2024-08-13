KERNEL_SRC=../linux-5.10
CUR_DIR=$(shell pwd)
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-
export ARCH CROSS_COMPILE

obj-m := mem_ctl.o

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(CUR_DIR) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(CUR_DIR) clean

copy:
	sudo mount ../rootfs.ext4 /mnt
	sudo cp *.ko /mnt
	sudo umount /mnt

.PHONE: clean copy
