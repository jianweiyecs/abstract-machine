#if __riscv_flen == 64
#define FP_S fsd
#define FP_L fld
#elif __riscv_flen == 32
#define FP_S fsw
#define FP_L flw
#endif
/*
 * M-mode trap entry.
 * Frame layout (uint64_t indices used by C):
 *   0..31  : x0..x31 (x0 unused / zero)
 *   32     : mepc
 *   33     : mstatus
 *   34     : mcause
 *   35     : mtval
 *   36..43: fcsr, FP valid, vlenb, vl, vtype, vstart, vcsr, V valid
 *   44..75: f0..f31 (8-byte slots)
 * Vector registers occupy 32*vlenb bytes below this 608-byte frame.
 */
    .section .text
    .globl m_trap_entry
    .align 2
m_trap_entry:
    csrrw   sp, mscratch, sp   /* sp = trap stack; mscratch = old sp */
    addi    sp, sp, -608

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

    mv      s0, sp
    sd      zero, 37*8(s0)
    sd      zero, 43*8(s0)
#ifdef __riscv_flen
    ld      t0, 33*8(s0)
    li      t1, 0x6000
    and     t0, t0, t1
    beqz    t0, 1f
    sd      t0, 37*8(s0)
    csrr    t0, fcsr
    sd      t0, 36*8(s0)
    FP_S    f0, 44*8(s0)
    FP_S    f1, 45*8(s0)
    FP_S    f2, 46*8(s0)
    FP_S    f3, 47*8(s0)
    FP_S    f4, 48*8(s0)
    FP_S    f5, 49*8(s0)
    FP_S    f6, 50*8(s0)
    FP_S    f7, 51*8(s0)
    FP_S    f8, 52*8(s0)
    FP_S    f9, 53*8(s0)
    FP_S    f10, 54*8(s0)
    FP_S    f11, 55*8(s0)
    FP_S    f12, 56*8(s0)
    FP_S    f13, 57*8(s0)
    FP_S    f14, 58*8(s0)
    FP_S    f15, 59*8(s0)
    FP_S    f16, 60*8(s0)
    FP_S    f17, 61*8(s0)
    FP_S    f18, 62*8(s0)
    FP_S    f19, 63*8(s0)
    FP_S    f20, 64*8(s0)
    FP_S    f21, 65*8(s0)
    FP_S    f22, 66*8(s0)
    FP_S    f23, 67*8(s0)
    FP_S    f24, 68*8(s0)
    FP_S    f25, 69*8(s0)
    FP_S    f26, 70*8(s0)
    FP_S    f27, 71*8(s0)
    FP_S    f28, 72*8(s0)
    FP_S    f29, 73*8(s0)
    FP_S    f30, 74*8(s0)
    FP_S    f31, 75*8(s0)
1:
#endif
#ifdef __riscv_vector
    ld      t0, 33*8(s0)
    andi    t0, t0, 0x600
    beqz    t0, 2f
    sd      t0, 43*8(s0)
    csrr    t0, vlenb
    sd      t0, 38*8(s0)
    csrr    t1, vl
    sd      t1, 39*8(s0)
    csrr    t1, vtype
    sd      t1, 40*8(s0)
    csrr    t1, vstart
    sd      t1, 41*8(s0)
    csrr    t1, vcsr
    sd      t1, 42*8(s0)
    slli    t1, t0, 5
    li      t2, TRAP_STACK_SIZE - 608 - 2048
    bgtu    t1, t2, trap_vector_too_large
    sub     sp, sp, t1
    csrwi   vstart, 0
    mv      t1, sp
    slli    t0, t0, 3
    vs8r.v  v0, (t1)
    add     t1, t1, t0
    vs8r.v  v8, (t1)
    add     t1, t1, t0
    vs8r.v  v16, (t1)
    add     t1, t1, t0
    vs8r.v  v24, (t1)
2:
#endif
    mv      a0, s0
    call    m_trap_handler

#ifdef __riscv_vector
    ld      t0, 43*8(s0)
    beqz    t0, 2f
    li      t0, 0x600
    csrs    mstatus, t0
    csrwi   vstart, 0
    ld      t0, 38*8(s0)
    slli    t0, t0, 3
    mv      t1, sp
    vl8re8.v v0, (t1)
    add     t1, t1, t0
    vl8re8.v v8, (t1)
    add     t1, t1, t0
    vl8re8.v v16, (t1)
    add     t1, t1, t0
    vl8re8.v v24, (t1)
    ld      t0, 39*8(s0)
    ld      t1, 40*8(s0)
    vsetvl  zero, t0, t1
    ld      t0, 42*8(s0)
    csrw    vcsr, t0
    ld      t0, 41*8(s0)
    csrw    vstart, t0
2:
#endif
#ifdef __riscv_flen
    ld      t0, 37*8(s0)
    beqz    t0, 1f
    li      t0, 0x6000
    csrs    mstatus, t0
    FP_L    f0, 44*8(s0)
    FP_L    f1, 45*8(s0)
    FP_L    f2, 46*8(s0)
    FP_L    f3, 47*8(s0)
    FP_L    f4, 48*8(s0)
    FP_L    f5, 49*8(s0)
    FP_L    f6, 50*8(s0)
    FP_L    f7, 51*8(s0)
    FP_L    f8, 52*8(s0)
    FP_L    f9, 53*8(s0)
    FP_L    f10, 54*8(s0)
    FP_L    f11, 55*8(s0)
    FP_L    f12, 56*8(s0)
    FP_L    f13, 57*8(s0)
    FP_L    f14, 58*8(s0)
    FP_L    f15, 59*8(s0)
    FP_L    f16, 60*8(s0)
    FP_L    f17, 61*8(s0)
    FP_L    f18, 62*8(s0)
    FP_L    f19, 63*8(s0)
    FP_L    f20, 64*8(s0)
    FP_L    f21, 65*8(s0)
    FP_L    f22, 66*8(s0)
    FP_L    f23, 67*8(s0)
    FP_L    f24, 68*8(s0)
    FP_L    f25, 69*8(s0)
    FP_L    f26, 70*8(s0)
    FP_L    f27, 71*8(s0)
    FP_L    f28, 72*8(s0)
    FP_L    f29, 73*8(s0)
    FP_L    f30, 74*8(s0)
    FP_L    f31, 75*8(s0)
    ld      t0, 36*8(s0)
    csrw    fcsr, t0
1:
#endif
    mv      sp, s0

    /* C may have updated mepc in frame[32] */
    ld      t0, 32*8(sp)
    csrw    mepc, t0
    ld      t0, 33*8(sp)
    csrw    mstatus, t0

    ld      ra, 1*8(sp)
    ld      gp, 3*8(sp)
    ld      tp, 4*8(sp)
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

    /* Restore sp without overwriting the interrupted t0. */
    ld      t0, 2*8(sp)
    csrw    mscratch, t0
    ld      t0, 5*8(sp)
    addi    sp, sp, 608
    csrrw   sp, mscratch, sp   /* mscratch = trap stack top for next trap */
    mret

#ifdef __riscv_vector
trap_vector_too_large:
    la      a0, trap_vector_stack_msg
    tail    test_skip
    .section .rodata
trap_vector_stack_msg:
    .asciz "VLEN exceeds configured TRAP_STACK_SIZE"
#endif
