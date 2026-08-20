#ifndef ARCH_RISCV_VECTOR_TEST_H
#define ARCH_RISCV_VECTOR_TEST_H

#include <arch/riscv-test.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXT_V_ENABLED

#define RVV_SENTINEL8  UINT8_C(0xa5)
#define RVV_SENTINEL16 UINT16_C(0xa55a)
#define RVV_SENTINEL32 UINT32_C(0xa55aa55a)
#define RVV_SENTINEL64 UINT64_C(0xa55aa55a5aa55aa5)

#define RVV_VTA (UINT64_C(1) << 6)
#define RVV_VMA (UINT64_C(1) << 7)
#define RVV_VSEW_8  (UINT64_C(0) << 3)
#define RVV_VSEW_16 (UINT64_C(1) << 3)
#define RVV_VSEW_32 (UINT64_C(2) << 3)
#define RVV_VSEW_64 (UINT64_C(3) << 3)
#define RVV_VLMUL_M1 UINT64_C(0)
#define RVV_VLMUL_M2 UINT64_C(1)
#define RVV_VLMUL_M4 UINT64_C(2)
#define RVV_VLMUL_M8 UINT64_C(3)
#define RVV_VLMUL_MF8 UINT64_C(5)
#define RVV_VLMUL_MF4 UINT64_C(6)
#define RVV_VLMUL_MF2 UINT64_C(7)

#define RVV_TRAP_CSR_UNAVAILABLE UINT64_MAX

typedef struct {
    uint64_t cause;
    uint64_t epc;
    uint64_t tval;
    uint64_t status;
    uint64_t vstart;
    uint64_t vl;
    uint64_t vtype;
} rvv_trap_snapshot_t;

/* Deterministic setup. This does not assume a particular VLEN. */
void rvv_test_init(void);
void rvv_reset_csrs(void);
uint64_t rvv_read_vlenb(void);
uint64_t rvv_setvl(uint64_t avl, uint64_t vtype);
uint64_t rvv_vlmax(uint64_t vtype);

/* Reproducible integer data; seed zero is valid and deterministic. */
uint64_t rvv_prng_next(uint64_t *state);
void rvv_fill_u8(uint8_t *dst, uint64_t n, uint64_t seed);
void rvv_fill_u16(uint16_t *dst, uint64_t n, uint64_t seed);
void rvv_fill_u32(uint32_t *dst, uint64_t n, uint64_t seed);
void rvv_fill_u64(uint64_t *dst, uint64_t n, uint64_t seed);
void rvv_fill_sentinel(void *dst, uint64_t n, unsigned element_bytes);

/* Bit-exact lane checks. FP tests pass IEEE-754 bit patterns to u32/u64. */
void rvv_check_u8(const uint8_t *got, const uint8_t *want, uint64_t n);
void rvv_check_u16(const uint16_t *got, const uint16_t *want, uint64_t n);
void rvv_check_u32(const uint32_t *got, const uint32_t *want, uint64_t n);
void rvv_check_u64(const uint64_t *got, const uint64_t *want, uint64_t n);
void rvv_check_sentinel(const void *got, uint64_t first, uint64_t n,
                        unsigned element_bytes);
void rvv_check_allowed_u8(const uint8_t *got, const uint8_t *a,
                          const uint8_t *b, uint64_t n);
void rvv_check_allowed_u16(const uint16_t *got, const uint16_t *a,
                           const uint16_t *b, uint64_t n);
void rvv_check_allowed_u32(const uint32_t *got, const uint32_t *a,
                           const uint32_t *b, uint64_t n);
void rvv_check_allowed_u64(const uint64_t *got, const uint64_t *a,
                           const uint64_t *b, uint64_t n);

/* Common cross-test setup helpers. */
uintptr_t rvv_page_base(const void *address);
void *rvv_page_offset(void *page_aligned, uint64_t offset);
void rvv_pmp_deny_page_allow_dram(unsigned deny_entry, unsigned allow_entry,
                                  uintptr_t page);
void rvv_get_trap_snapshot(rvv_trap_snapshot_t *snapshot);

static inline uint64_t rvv_read_vxrm(void) {
    return csr_read(vxrm);
}

static inline void rvv_write_vxrm(uint64_t value) {
    csr_write(vxrm, value);
}

static inline uint64_t rvv_read_vxsat(void) {
    return csr_read(vxsat);
}

static inline void rvv_write_vxsat(uint64_t value) {
    csr_write(vxsat, value);
}

static inline uint64_t rvv_read_fflags(void) {
    return csr_read(fflags);
}

static inline void rvv_write_fflags(uint64_t value) {
    csr_write(fflags, value);
}

#endif /* EXT_V_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* ARCH_RISCV_VECTOR_TEST_H */
