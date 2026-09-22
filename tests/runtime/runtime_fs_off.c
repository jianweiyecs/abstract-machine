#include <arch/riscv-test.h>

#if __riscv_flen == 64
#define FP_FROM_INT "fmv.d.x "
#define FP_TO_INT "fmv.x.d "
#define FP_BITS UINT64_C(0x400921fb54442d18)
#elif __riscv_flen == 32
#define FP_FROM_INT "fmv.w.x "
#define FP_TO_INT "fmv.x.w "
#define FP_BITS UINT64_C(0x40490fdb)
#else
#error "This regression requires floating-point support"
#endif

static void enable_fp(void) {
    TEST_ASSERT_EQ(trap_last_status() & MSTATUS_FS, 0);
    csr_set(mstatus, MSTATUS_FS);
}

int main(void) {
    uint64_t got, initial = FP_BITS;
    trap_on(CAUSE_ILLEGAL_INSTRUCTION, 0, 0, TRAP_RETRY, enable_fp);
    csr_clear(mstatus, MSTATUS_FS);
    __asm__ volatile(FP_FROM_INT "ft0,%1\n" FP_TO_INT "%0,ft0"
                     : "=r"(got) : "r"(initial) : "ft0", "memory");
    TEST_ASSERT(trap_was_taken());
    TEST_ASSERT_EQ(got, initial);
    test_pass();
}
