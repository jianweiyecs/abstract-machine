    .section .text.init, "ax", @progbits
    .globl _test_start
    .align 2
_test_start:
    /* Park non-zero harts. */
    csrr    t0, mhartid
    bnez    t0, 3f

    /* Global pointer (needed for .sdata/.sbss even with -mno-relax). */
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    la      sp, __stack_top
    andi    sp, sp, -16

    /* Clear BSS */
    la      t0, __bss_start
    la      t1, __bss_end
1:
    bgeu    t0, t1, 2f
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       1b
2:
    /* mscratch = M-mode trap stack top */
    la      t0, __mtrap_stack_top
    csrw    mscratch, t0

    /* Direct mode mtvec */
    la      t0, m_trap_entry
    csrw    mtvec, t0

    /* Disable interrupts for bare-metal tests */
    csrci   mstatus, 0x8
    csrw    mie, zero

    /*
     * Enable the FPU: mstatus.FS must be non-Off, otherwise any FP
     * instruction traps as illegal (cause=2). Set FS=Dirty (0b11).
     * Bits [14:13] = 0b11 -> 0x6000.
     */
    li      t0, 0x6000
    csrs    mstatus, t0
    csrwi   fcsr, 0

    call    trap_init
    call    main

    /* main returned without test_pass/test_fail */
    li      a0, 1
    call    halt

3:
    wfi
    j       3b
