OUTPUT_ARCH(riscv)
ENTRY(_test_start)

MEMORY
{
    RAM (rwx) : ORIGIN = CONFIG_DRAM_BASE, LENGTH = CONFIG_DRAM_SIZE
}

SECTIONS
{
    . = ORIGIN(RAM);

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
        . = ALIGN(8);
        __bss_end = .;
    } > RAM

    .stacks (NOLOAD) : ALIGN(16) {
        __stack_bottom = .;
        . += CONFIG_HARTS * CONFIG_STACK_SIZE;
        __stack_top = .;
        PROVIDE(__stack_size = CONFIG_STACK_SIZE);
        __mtrap_stack_bottom = .;
        . += CONFIG_HARTS * CONFIG_TRAP_STACK_SIZE;
        __mtrap_stack_top = .;
        PROVIDE(__mtrap_stack_size = CONFIG_TRAP_STACK_SIZE);
    } > RAM

    .page_arena (NOLOAD) : ALIGN(4096) {
        __page_arena_start = .;
        . += CONFIG_PAGE_ARENA_SIZE;
        __page_arena_end = .;
    } > RAM
    __heap_start = .;
    __heap_end = ORIGIN(RAM) + LENGTH(RAM);
    _end = .;
    ASSERT(CONFIG_HARTS >= 1, "HARTS must be positive")
    ASSERT(CONFIG_STACK_SIZE >= 4096 && CONFIG_STACK_SIZE % 16 == 0,
           "STACK_SIZE must be >= 4096 and aligned to 16")
    ASSERT(CONFIG_TRAP_STACK_SIZE >= 4096 && CONFIG_TRAP_STACK_SIZE % 16 == 0,
           "TRAP_STACK_SIZE must be >= 4096 and aligned to 16")
    ASSERT(CONFIG_PAGE_ARENA_SIZE % 4096 == 0, "page arena must be page aligned")
    ASSERT(__heap_start <= __heap_end, "runtime reservations exceed DRAM")
}
