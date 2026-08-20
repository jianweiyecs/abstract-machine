#include <arch/riscv-vector-test.h>

#ifdef EXT_V_ENABLED

static void check_element_bytes(unsigned element_bytes) {
    if (element_bytes != 1 && element_bytes != 2 && element_bytes != 4 &&
        element_bytes != 8)
        test_fail("rvv element size");
}

void rvv_reset_csrs(void) {
    vector_write_vstart(0);
    rvv_write_vxrm(0);
    rvv_write_vxsat(0);
    rvv_write_fflags(0);
}

void rvv_test_init(void) {
    if (!vector_available())
        test_fail("no misa.V");
    vector_enable();
    rvv_reset_csrs();
}

uint64_t rvv_read_vlenb(void) {
    return csr_read(vlenb);
}

uint64_t rvv_setvl(uint64_t avl, uint64_t vtype) {
    uint64_t vl;

    __asm__ volatile("vsetvl %0, %1, %2"
                     : "=r"(vl)
                     : "r"(avl), "r"(vtype)
                     : "memory");
    return vl;
}

uint64_t rvv_vlmax(uint64_t vtype) {
    return rvv_setvl(UINT64_MAX, vtype);
}

uint64_t rvv_prng_next(uint64_t *state) {
    uint64_t x = *state;

    if (x == 0)
        x = UINT64_C(0x9e3779b97f4a7c15);
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

void rvv_fill_u8(uint8_t *dst, uint64_t n, uint64_t seed) {
    uint64_t i;

    for (i = 0; i < n; i++)
        dst[i] = (uint8_t)rvv_prng_next(&seed);
}

void rvv_fill_u16(uint16_t *dst, uint64_t n, uint64_t seed) {
    uint64_t i;

    for (i = 0; i < n; i++)
        dst[i] = (uint16_t)rvv_prng_next(&seed);
}

void rvv_fill_u32(uint32_t *dst, uint64_t n, uint64_t seed) {
    uint64_t i;

    for (i = 0; i < n; i++)
        dst[i] = (uint32_t)rvv_prng_next(&seed);
}

void rvv_fill_u64(uint64_t *dst, uint64_t n, uint64_t seed) {
    uint64_t i;

    for (i = 0; i < n; i++)
        dst[i] = rvv_prng_next(&seed);
}

void rvv_fill_sentinel(void *dst, uint64_t n, unsigned element_bytes) {
    uint64_t i;

    check_element_bytes(element_bytes);
    for (i = 0; i < n; i++) {
        switch (element_bytes) {
        case 1:
            ((uint8_t *)dst)[i] = RVV_SENTINEL8;
            break;
        case 2:
            ((uint16_t *)dst)[i] = RVV_SENTINEL16;
            break;
        case 4:
            ((uint32_t *)dst)[i] = RVV_SENTINEL32;
            break;
        default:
            ((uint64_t *)dst)[i] = RVV_SENTINEL64;
            break;
        }
    }
}

#define DEFINE_CHECK(bits, type)                                                \
    void rvv_check_u##bits(const type *got, const type *want, uint64_t n) {     \
        uint64_t i;                                                              \
        for (i = 0; i < n; i++)                                                  \
            if (got[i] != want[i]) {                                             \
                test_puts("RVV lane mismatch lane=");                           \
                test_put_hex(i);                                                 \
                test_puts(" got=");                                             \
                test_put_hex((uint64_t)got[i]);                                  \
                test_puts(" want=");                                            \
                test_put_hex((uint64_t)want[i]);                                 \
                test_puts("\n");                                                \
                test_fail("rvv lane");                                          \
            }                                                                    \
    }                                                                            \
    void rvv_check_allowed_u##bits(const type *got, const type *a,              \
                                    const type *b, uint64_t n) {                  \
        uint64_t i;                                                              \
        for (i = 0; i < n; i++)                                                  \
            if (got[i] != a[i] && got[i] != b[i]) {                              \
                test_puts("RVV disallowed lane=");                              \
                test_put_hex(i);                                                 \
                test_puts(" got=");                                             \
                test_put_hex((uint64_t)got[i]);                                  \
                test_puts("\n");                                                \
                test_fail("rvv allowed set");                                   \
            }                                                                    \
    }

DEFINE_CHECK(8, uint8_t)
DEFINE_CHECK(16, uint16_t)
DEFINE_CHECK(32, uint32_t)
DEFINE_CHECK(64, uint64_t)

void rvv_check_sentinel(const void *got, uint64_t first, uint64_t n,
                        unsigned element_bytes) {
    uint64_t i;

    check_element_bytes(element_bytes);
    for (i = first; i < first + n; i++) {
        uint64_t value;
        uint64_t sentinel;

        switch (element_bytes) {
        case 1:
            value = ((const uint8_t *)got)[i];
            sentinel = RVV_SENTINEL8;
            break;
        case 2:
            value = ((const uint16_t *)got)[i];
            sentinel = RVV_SENTINEL16;
            break;
        case 4:
            value = ((const uint32_t *)got)[i];
            sentinel = RVV_SENTINEL32;
            break;
        default:
            value = ((const uint64_t *)got)[i];
            sentinel = RVV_SENTINEL64;
            break;
        }
        if (value != sentinel) {
            test_puts("RVV sentinel changed lane=");
            test_put_hex(i);
            test_puts(" got=");
            test_put_hex(value);
            test_puts("\n");
            test_fail("rvv sentinel");
        }
    }
}

uintptr_t rvv_page_base(const void *address) {
    return (uintptr_t)address & ~(uintptr_t)(PAGE_SIZE - 1);
}

void *rvv_page_offset(void *page_aligned, uint64_t offset) {
    TEST_ASSERT((((uintptr_t)page_aligned) & (PAGE_SIZE - 1)) == 0);
    TEST_ASSERT(offset < PAGE_SIZE);
    return (void *)((uintptr_t)page_aligned + offset);
}

void rvv_pmp_deny_page_allow_dram(unsigned deny_entry, unsigned allow_entry,
                                  uintptr_t page) {
    TEST_ASSERT((page & (PAGE_SIZE - 1)) == 0);
    TEST_ASSERT(deny_entry != allow_entry);
    pmp_clear_all();
    pmp_set_napot(deny_entry, page, PAGE_SIZE, PMP_A_NAPOT);
    pmp_set_napot(allow_entry, DRAM_BASE, DRAM_SIZE,
                  PMP_R | PMP_W | PMP_X | PMP_A_NAPOT);
}

void rvv_get_trap_snapshot(rvv_trap_snapshot_t *snapshot) {
    TEST_ASSERT(snapshot != 0);
    snapshot->cause = trap_last_cause();
    snapshot->epc = trap_last_epc();
    snapshot->tval = trap_last_tval();
    snapshot->status = trap_last_status();
    snapshot->vstart = trap_last_vstart();
    snapshot->vl = trap_last_vl();
    snapshot->vtype = trap_last_vtype();
}

#endif /* EXT_V_ENABLED */
