#include <arch/riscv-test.h>

#if __riscv_flen == 64
#define FP_FROM_INT "fmv.d.x "
#define FP_TO_INT "fmv.x.d "
#define FP_BITS UINT64_C(0x400921fb54442d18)
#define FP_ONE UINT64_C(0x3ff0000000000000)
#define FP_TWO UINT64_C(0x4000000000000000)
#define FP_DOUBLE "fadd.d ft0,ft0,ft0,dyn\n"
#elif __riscv_flen == 32
#define FP_FROM_INT "fmv.w.x "
#define FP_TO_INT "fmv.x.w "
#define FP_BITS UINT64_C(0x40490fdb)
#define FP_ONE UINT64_C(0x3f800000)
#define FP_TWO UINT64_C(0x40000000)
#define FP_DOUBLE "fadd.s ft0,ft0,ft0,dyn\n"
#else
#error "This regression requires floating-point support"
#endif

static void resolve(void) {
    __asm__ volatile(FP_FROM_INT "ft0,zero\n" FP_FROM_INT "ft11,zero"
                     ::: "ft0", "ft11");
    csr_write(fcsr, 0);
}

static void repair_rounding(void) {
    csr_write(frm, 0);
    trap_commit_fcsr();
}

int main(void) {
    uint64_t first, last, initial = FP_BITS;
    trap_on(CAUSE_MACHINE_ECALL, 0, 0, TRAP_SKIP, resolve);
    csr_write(fcsr, 0x65);
    __asm__ volatile(FP_FROM_INT "ft0,%2\n" FP_FROM_INT "ft11,%2\n"
                     "ecall\n"
                     FP_TO_INT "%0,ft0\n" FP_TO_INT "%1,ft11"
                     : "=r"(first), "=r"(last) : "r"(initial)
                     : "ft0", "ft11", "memory");
    TEST_ASSERT_EQ(csr_read(fcsr), 0x65);
    TEST_ASSERT(trap_was_taken());
    TEST_ASSERT_EQ(first, initial);
    TEST_ASSERT_EQ(last, initial);

    trap_on(CAUSE_MACHINE_ECALL, 0, 0, TRAP_SKIP, repair_rounding);
    csr_write(fcsr, 3 << 5);
    __asm__ volatile(FP_FROM_INT "ft0,%1\necall\n" FP_DOUBLE FP_TO_INT "%0,ft0"
                     : "=r"(first) : "r"(FP_ONE) : "ft0", "memory");
    TEST_ASSERT(trap_was_taken());
    TEST_ASSERT_EQ(csr_read(fcsr), 0);
    TEST_ASSERT_EQ(first, FP_TWO);
    test_pass();
}
