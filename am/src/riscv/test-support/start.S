    .section .text.init, "ax", @progbits
    .globl _test_start
    .align 2
_test_start:
    /* Harts >= HART_MAX stay parked. tp = hart id for C hart_id(). */
    csrr    tp, mhartid
    li      t1, 4
    bgeu    tp, t1, 4f

    /* Global pointer (needed for .sdata/.sbss even with -mno-relax). */
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    /* Per-hart main stack: __stack_top - hartid * 64KiB (must match test.ld). */
    la      sp, __stack_top
    li      t1, 0x10000
    csrr    t0, mhartid
    mul     t0, t0, t1
    sub     sp, sp, t0
    andi    sp, sp, -16

    /* Per-hart M-mode trap stack into mscratch (16KiB / hart). */
    la      t2, __mtrap_stack_top
    li      t1, 0x4000
    csrr    t0, mhartid
    mul     t0, t0, t1
    sub     t2, t2, t0
    andi    t2, t2, -16
    csrw    mscratch, t2

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

#ifdef __riscv_vector
    /* Every hart initializes all VLEN bits before main or a mailbox worker. */
    call    vector_init
#endif

    csrr    t0, mhartid
    bnez    t0, 5f

    /* Hart 0: clear BSS, publish boot_ready (in .data, not BSS). */
    la      t0, __bss_start
    la      t1, __bss_end
1:
    bgeu    t0, t1, 2f
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       1b
2:
    call    trap_init
    li      t0, 1
    la      t1, hart_boot_ready
    fence   rw, rw
    sd      t0, 0(t1)
    fence   rw, rw
    call    main

    /* main returned without test_pass/test_fail */
    li      a0, 1
    call    halt

    /* Secondary: wait until BSS is done, then poll mailbox[id]. */
5:
    la      t1, hart_boot_ready
6:
    ld      t0, 0(t1)
    beqz    t0, 6b
    fence   rw, rw

7:
    la      t1, hart_mailbox
    csrr    t2, mhartid
    slli    t3, t2, 3
    add     t1, t1, t3
    ld      t0, 0(t1)
    beqz    t0, 7b
    /* Mark running before clearing mailbox so hart_start cannot race. */
    la      t4, hart_running
    add     t4, t4, t3
    li      t5, 1
    sd      t5, 0(t4)
    fence   rw, rw
    sd      zero, 0(t1)
    fence   rw, rw
    jalr    t0
    fence   rw, rw
    csrr    t2, mhartid
    slli    t3, t2, 3
    la      t1, hart_running
    add     t1, t1, t3
    sd      zero, 0(t1)
    la      t1, hart_done
    add     t1, t1, t3
    li      t4, 1
    sd      t4, 0(t1)
    fence   rw, rw
    j       7b

4:
    wfi
    j       4b
