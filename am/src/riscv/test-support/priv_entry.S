#if __riscv_flen == 64
#define FP_S fsd
#define FP_L fld
#elif __riscv_flen == 32
#define FP_S fsw
#define FP_L flw
#endif
    .section .text
    .align 2
    .globl priv_enter_s
priv_enter_s:
    li      t3, 0x0800
    j       priv_enter
    .globl priv_enter_u
priv_enter_u:
    li      t3, 0
priv_enter:
    csrr    t0, mhartid
    li      t1, HART_BASE
    bne     t0, t1, priv_bad_hart
    la      t0, priv_m_ctx
    sd      ra, 0*8(t0)
    sd      sp, 1*8(t0)
    sd      gp, 2*8(t0)
    sd      tp, 3*8(t0)
    sd      s0, 4*8(t0)
    sd      s1, 5*8(t0)
    sd      s2, 6*8(t0)
    sd      s3, 7*8(t0)
    sd      s4, 8*8(t0)
    sd      s5, 9*8(t0)
    sd      s6, 10*8(t0)
    sd      s7, 11*8(t0)
    sd      s8, 12*8(t0)
    sd      s9, 13*8(t0)
    sd      s10, 14*8(t0)
    sd      s11, 15*8(t0)

#ifdef __riscv_flen
    csrr    t1, mstatus
    li      t2, 0x6000
    and     t1, t1, t2
    sd      t1, 16*8(t0)
    beqz    t1, 1f
    FP_S    fs0, 17*8(t0)
    FP_S    fs1, 18*8(t0)
    FP_S    fs2, 19*8(t0)
    FP_S    fs3, 20*8(t0)
    FP_S    fs4, 21*8(t0)
    FP_S    fs5, 22*8(t0)
    FP_S    fs6, 23*8(t0)
    FP_S    fs7, 24*8(t0)
    FP_S    fs8, 25*8(t0)
    FP_S    fs9, 26*8(t0)
    FP_S    fs10, 27*8(t0)
    FP_S    fs11, 28*8(t0)
1:
#endif
    csrr    t1, mstatus
    li      t2, ~0x21800
    and     t1, t1, t2
    or      t1, t1, t3
    csrw    mstatus, t1
    csrw    mepc, a0
    mret

    .globl priv_resume_m
priv_resume_m:
    la      t0, priv_m_ctx
#ifdef __riscv_flen
    ld      t1, 16*8(t0)
    beqz    t1, 1f
    li      t2, 0x6000
    csrs    mstatus, t2
    FP_L    fs0, 17*8(t0)
    FP_L    fs1, 18*8(t0)
    FP_L    fs2, 19*8(t0)
    FP_L    fs3, 20*8(t0)
    FP_L    fs4, 21*8(t0)
    FP_L    fs5, 22*8(t0)
    FP_L    fs6, 23*8(t0)
    FP_L    fs7, 24*8(t0)
    FP_L    fs8, 25*8(t0)
    FP_L    fs9, 26*8(t0)
    FP_L    fs10, 27*8(t0)
    FP_L    fs11, 28*8(t0)
1:
#endif
    ld      ra, 0*8(t0)
    ld      sp, 1*8(t0)
    ld      gp, 2*8(t0)
    ld      tp, 3*8(t0)
    ld      s0, 4*8(t0)
    ld      s1, 5*8(t0)
    ld      s2, 6*8(t0)
    ld      s3, 7*8(t0)
    ld      s4, 8*8(t0)
    ld      s5, 9*8(t0)
    ld      s6, 10*8(t0)
    ld      s7, 11*8(t0)
    ld      s8, 12*8(t0)
    ld      s9, 13*8(t0)
    ld      s10, 14*8(t0)
    ld      s11, 15*8(t0)
    ret


priv_bad_hart:
    la      a0, priv_bad_hart_msg
    tail    test_fail
    .section .rodata
priv_bad_hart_msg:
    .asciz "priv_enter requires logical hart 0"
    .bss
    .align 3
priv_m_ctx:
    .space 29*8
