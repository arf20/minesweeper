GNUEFIPATH=/usr/include/efi

#MBRSRCDIR		:= 
MBROBJDIR	:= obj_x86
EFIOBJDIR	:= obj_x86_64

AS=nasm
CC=gcc
LD=ld
CFLAGS=-Wall -Wextra -Wpedantic -m32 -ffreestanding -nostdlib -fno-pie -DFRONTENDS_KERNEL \
	-I$(GNUEFIPATH) -I$(GNUEFIPATH)/x86_64 -I$(GNUEFIPATH)/protocol

MBRSRC := \
	kernel_entry.c \
	kernel.c \
	port.c \
	vgacon.c \
	keyb.c \
	alloc.c \
	random.c \
	rtc_time.c \
	int32.c \
	plibc.c \
	../common/game.c \
	kfrontends/vgacli.c \
	kfrontends/vgatui.c

MBROBJ := $(patsubst $(SRCDIR)/%,$(MBROBJDIR)/%,$(MBRSRC:.$(SRCEXT)=.$(OBJEXT)))

all: arfminesweeper.bin

dirs:
	@mkdir -p $(build_x86)

mbr_boot.bin: mbr_boot.asm
	$(AS) $< -f bin -o $@

kernel_entry.o: kernel_entry.asm
	$(AS) $< -f elf -o $@

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.asm $(DEPS)
	$(AS) $< -f elf -o $@

kernel.bin: kernel_entry.o kernel.o port.o vgacon.o keyb.o alloc.o random.o rtc_time.o int32.o plibc.o ../common/game.o kfrontends/vgacli.o kfrontends/vgatui.o 
	$(LD) -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

arfminesweeper.bin: mbr_boot.bin kernel.bin
	cat $^ > $@ && truncate -s 64K arfminesweeper.bin

BOOTX64.EFI: efi_kernel.o
	x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o BOOTX64.EFI efi_kernel.o

run: arfminesweeper.bin
	qemu-system-i386 -hda $<

clean:
	$(RM) *.bin *.o kfrontends/*.o *.dis
