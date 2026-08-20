/*
 * M-mode trap entry.
 * Frame layout (uint64_t indices used by C):
 *   0..31  : x0..x31 (x0 unused / zero)
 *   32     : mepc
 *   33     : mstatus
 *   34     : mcause
 *   35     : mtval
 * Total 36 * 8 = 288 bytes
 */
    .section .text
    .globl m_trap_entry
    .align 2
m_trap_entry:
    csrrw   sp, mscratch, sp   /* sp = trap stack; mscratch = old sp */
    addi    sp, sp, -288

    sd      ra, 1*8(sp)
    /* x2/sp saved later from mscratch */
    sd      gp, 3*8(sp)
    sd      tp, 4*8(sp)
    sd      t0, 5*8(sp)
    sd      t1, 6*8(sp)
    sd      t2, 7*8(sp)
    sd      s0, 8*8(sp)
    sd      s1, 9*8(sp)
    sd      a0, 10*8(sp)
    sd      a1, 11*8(sp)
    sd      a2, 12*8(sp)
    sd      a3, 13*8(sp)
    sd      a4, 14*8(sp)
    sd      a5, 15*8(sp)
    sd      a6, 16*8(sp)
    sd      a7, 17*8(sp)
    sd      s2, 18*8(sp)
    sd      s3, 19*8(sp)
    sd      s4, 20*8(sp)
    sd      s5, 21*8(sp)
    sd      s6, 22*8(sp)
    sd      s7, 23*8(sp)
    sd      s8, 24*8(sp)
    sd      s9, 25*8(sp)
    sd      s10, 26*8(sp)
    sd      s11, 27*8(sp)
    sd      t3, 28*8(sp)
    sd      t4, 29*8(sp)
    sd      t5, 30*8(sp)
    sd      t6, 31*8(sp)

    csrr    t0, mscratch
    sd      t0, 2*8(sp)       /* original sp */

    csrr    t0, mepc
    sd      t0, 32*8(sp)
    csrr    t0, mstatus
    sd      t0, 33*8(sp)
    csrr    t0, mcause
    sd      t0, 34*8(sp)
    csrr    t0, mtval
    sd      t0, 35*8(sp)

    mv      a0, sp
    call    m_trap_handler

    /* C may have updated mepc in frame[32] */
    ld      t0, 32*8(sp)
    csrw    mepc, t0
    ld      t0, 33*8(sp)
    csrw    mstatus, t0

    ld      ra, 1*8(sp)
    ld      gp, 3*8(sp)
    ld      tp, 4*8(sp)
    ld      t0, 5*8(sp)
    ld      t1, 6*8(sp)
    ld      t2, 7*8(sp)
    ld      s0, 8*8(sp)
    ld      s1, 9*8(sp)
    ld      a0, 10*8(sp)
    ld      a1, 11*8(sp)
    ld      a2, 12*8(sp)
    ld      a3, 13*8(sp)
    ld      a4, 14*8(sp)
    ld      a5, 15*8(sp)
    ld      a6, 16*8(sp)
    ld      a7, 17*8(sp)
    ld      s2, 18*8(sp)
    ld      s3, 19*8(sp)
    ld      s4, 20*8(sp)
    ld      s5, 21*8(sp)
    ld      s6, 22*8(sp)
    ld      s7, 23*8(sp)
    ld      s8, 24*8(sp)
    ld      s9, 25*8(sp)
    ld      s10, 26*8(sp)
    ld      s11, 27*8(sp)
    ld      t3, 28*8(sp)
    ld      t4, 29*8(sp)
    ld      t5, 30*8(sp)
    ld      t6, 31*8(sp)

    /* restore original sp into mscratch temp then swap back */
    ld      t0, 2*8(sp)
    addi    sp, sp, 288
    csrw    mscratch, sp       /* restore trap stack top for next trap */
    mv      sp, t0
    mret
