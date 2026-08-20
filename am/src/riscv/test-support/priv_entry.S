/*
 * Privilege entry trampoline.
 *
 * priv_enter_s / priv_enter_u save M-mode C context, mret into the payload,
 * and return to the C caller only after the expected trap path jumps to
 * priv_resume_m (installed by the trap handler when recovering from S/U).
 */
    .section .text
    .align 2

    .globl priv_enter_s
priv_enter_s:
    /* a0 = void (*fn)(void) */
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

    /* mstatus.MPP = S */
    csrr    t1, mstatus
    li      t2, ~0x1800
    and     t1, t1, t2
    li      t2, 0x0800
    or      t1, t1, t2
    li      t2, ~0x20000
    and     t1, t1, t2          /* clear MPRV */
    csrw    mstatus, t1
    csrw    mepc, a0
    mret
    /* not reached; return is via priv_resume_m */

    .globl priv_enter_u
priv_enter_u:
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

    /* mstatus.MPP = U (0) */
    csrr    t1, mstatus
    li      t2, ~0x1800
    and     t1, t1, t2
    li      t2, ~0x20000
    and     t1, t1, t2
    csrw    mstatus, t1
    csrw    mepc, a0
    mret

    .globl priv_resume_m
priv_resume_m:
    la      t0, priv_m_ctx
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

    .bss
    .align 3
    .globl priv_m_ctx
priv_m_ctx:
    .space 16*8
