    .section .text
    .align 2
    .globl vector_init
    .type vector_init, @function
vector_init:
    csrr    t0, misa
    li      t1, (1 << 21)
    and     t0, t0, t1
    beqz    t0, 1f

    csrr    t2, mstatus
    li      t1, (3 << 9)
    csrs    mstatus, t1
    csrwi   vstart, 0

    /* rs1=x0, rd!=x0 selects VLMAX, independent of the incoming vl.
     * Four LMUL=8 groups cover every bit of v0..v31, including tails. */
    vsetvli t0, zero, e8, m8, tu, mu
    vmv.v.i v0, 0
    vmv.v.i v8, 0
    vmv.v.i v16, 0
    vmv.v.i v24, 0

    /* Deterministic reset configuration; preserve the incoming VS/FS state. */
    li      t0, (1 << 63)
    vsetvl  zero, zero, t0
    csrwi   vstart, 0
    csrwi   vcsr, 0
    csrw    mstatus, t2
1:
    ret
    .size vector_init, .-vector_init
