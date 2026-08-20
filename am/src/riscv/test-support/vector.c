#include <arch/riscv-test.h>

#ifdef EXT_V_ENABLED

void vector_enable(void) {
    uint64_t mstatus = csr_read(mstatus);

    mstatus = (mstatus & ~MSTATUS_VS) | MSTATUS_VS_INITIAL;
    csr_write(mstatus, mstatus);
}

void vector_disable(void) {
    uint64_t mstatus = csr_read(mstatus);

    mstatus = (mstatus & ~MSTATUS_VS) | MSTATUS_VS_OFF;
    csr_write(mstatus, mstatus);
}

void vector_add_i32(const int32_t *src_a, const int32_t *src_b, int32_t *dst,
                    uint64_t n) {
    uint64_t vl;
    const int32_t *a = src_a;
    const int32_t *b = src_b;
    int32_t *d = dst;
    uint64_t left = n;

    while (left > 0) {
        vl = vector_vsetvli_e32m1(left);
        __asm__ volatile(
            "vle32.v v0, (%0)\n"
            "vle32.v v1, (%1)\n"
            "vadd.vv v2, v0, v1\n"
            "vse32.v v2, (%2)\n"
            :
            : "r"(a), "r"(b), "r"(d)
            : "memory");
        a += vl;
        b += vl;
        d += vl;
        left -= vl;
    }
}

#endif /* EXT_V_ENABLED */
