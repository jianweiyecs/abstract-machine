#include <arch/riscv-test.h>

static unsigned char before[32 * 256] __attribute__((aligned(16)));
static unsigned char after[32 * 256] __attribute__((aligned(16)));

static void clobber(void) {
    TEST_ASSERT_EQ(trap_last_vstart(), 3);
    TEST_ASSERT_EQ(trap_last_vl(), 7);
    __asm__ volatile("vsetvli t0,zero,e8,m8,ta,ma\n"
                     "vmv.v.i v0,0\nvmv.v.i v8,0\n"
                     "vmv.v.i v16,0\nvmv.v.i v24,0\n"
                     "csrwi vcsr,0\n" ::: "t0", "memory");
}

int main(void) {
    TEST_REQUIRE(vector_available(), "requires V");
    vector_enable();
    unsigned bytes = csr_read(vlenb);
    TEST_REQUIRE(bytes <= 256, "regression buffer supports VLEN <= 2048");
    for (unsigned i = 0; i < 32 * bytes; i++)
        before[i] = (i * 13 + i / bytes + 1) & 255;
    trap_on(CAUSE_MACHINE_ECALL, 0, 0, TRAP_SKIP, clobber);
    unsigned long vl, vtype, start, vcsr;
    __asm__ volatile(
        "vl8re8.v v0,(%5)\nvl8re8.v v8,(%6)\n"
        "vl8re8.v v16,(%7)\nvl8re8.v v24,(%8)\n"
        "vsetivli zero,7,e8,m1,tu,mu\n"
        "csrwi vcsr,5\ncsrwi vstart,3\necall\n"
        "csrr %0,vl\ncsrr %1,vtype\ncsrr %2,vstart\ncsrr %3,vcsr\n"
        "csrwi vstart,0\n"
        "vs8r.v v0,(%9)\nvs8r.v v8,(%10)\n"
        "vs8r.v v16,(%11)\nvs8r.v v24,(%12)\n"
        : "=&r"(vl), "=&r"(vtype), "=&r"(start), "=&r"(vcsr), "=m"(after)
        : "r"(before), "r"(before + 8 * bytes), "r"(before + 16 * bytes),
          "r"(before + 24 * bytes), "r"(after), "r"(after + 8 * bytes),
          "r"(after + 16 * bytes), "r"(after + 24 * bytes)
        : "memory");
    TEST_ASSERT_EQ(vl, 7);
    TEST_ASSERT_EQ(vtype, 0);
    TEST_ASSERT_EQ(start, 3);
    TEST_ASSERT_EQ(vcsr, 5);
    for (unsigned i = 0; i < 32 * bytes; i++)
        TEST_ASSERT_EQ(after[i], before[i]);
    test_pass();
}
