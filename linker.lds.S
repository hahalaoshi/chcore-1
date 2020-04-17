#include "../boot/image.h"

SECTIONS
{
    . = TEXT_OFFSET;
    img_start = .;
    init : {
        CMakeFiles/kernel.img.dir/boot/start.S.o
     CMakeFiles/kernel.img.dir/boot/mmu.c.o
     CMakeFiles/kernel.img.dir/boot/tools.S.o
     CMakeFiles/kernel.img.dir/boot/init_c.c.o
     CMakeFiles/kernel.img.dir/boot/print/macro.S.o
     CMakeFiles/kernel.img.dir/boot/print/uart.c.o
     CMakeFiles/kernel.img.dir/boot/print/printf.c.o
    }

    . = ALIGN(SZ_16K);

    init_end = ABSOLUTE(.);

    .text KERNEL_VADDR + init_end : AT(init_end) {
        *(.text*)
        . = ALIGN(SZ_16);
    }

    . = ALIGN(SZ_64K);
    .data : {
        *(.data*)
        . = ALIGN(SZ_16);
    }
    . = ALIGN(SZ_64K);

    .rodata : {
        *(.rodata*)
        . = ALIGN(SZ_16);
    }
    _edata = . - KERNEL_VADDR;

    _bss_start = . - KERNEL_VADDR;
    .bss : {
        *(.bss*)
        . = ALIGN(SZ_16);
    }
    _bss_end = . - KERNEL_VADDR;
    . = ALIGN(SZ_64K);
    img_end = . - KERNEL_VADDR;
}
