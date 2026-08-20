OUTPUT_ARCH(riscv)
ENTRY(_test_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.init)
        *(.text .text.*)
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > RAM

    .data : {
        *(.data .data.*)
        . = ALIGN(8);
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.*)
    } > RAM

    /* HTIF tohost for Spike exit */
    .tohost : ALIGN(0x1000) {
        *(.tohost)
    } > RAM

    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    . = ALIGN(16);
    __stack_bottom = .;
    . += 0x10000; /* 64 KiB main stack */
    __stack_top = .;

    . = ALIGN(16);
    __mtrap_stack_bottom = .;
    . += 0x4000; /* 16 KiB M-mode trap stack */
    __mtrap_stack_top = .;

    . = ALIGN(4096);
    __page_arena_start = .;
    . += 0x40000; /* 256 KiB page table arena */
    __page_arena_end = .;

    __heap_start = .;
    _end = .;
}
