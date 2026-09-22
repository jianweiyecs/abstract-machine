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

static void lower(void) {
    __asm__ volatile(FP_FROM_INT "fs0, zero" ::: "fs0");
    __asm__ volatile("ecall" ::: "memory");
}
int main(void) {
    uint64_t got, initial = FP_BITS;
    priv_allow_dram();
    trap_expect(CAUSE_SUPERVISOR_ECALL, 0, 0, 0, 0);
    __asm__ volatile(FP_FROM_INT "fs0, %0" :: "r"(initial) : "fs0");
    priv_enter_s(lower);
    __asm__ volatile(FP_TO_INT "%0, fs0" : "=r"(got));
    TEST_ASSERT(trap_was_taken());
    TEST_ASSERT_EQ(got, initial);
    test_pass();
}
