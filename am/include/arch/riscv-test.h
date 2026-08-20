#ifndef ARCH_RISCV_TEST_H
#define ARCH_RISCV_TEST_H

#include <stdint.h>
#include <stdbool.h>

/*
 * RISC-V Test Environment API
 * Provides bare-metal testing support for RISC-V processors:
 * - Privilege level switching (M/S/U)
 * - Trap expectation and verification
 * - PMP configuration
 * - Page table management (Sv39)
 * - Extension support (Vector, Hypervisor, etc.)
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CSR Access Macros
 * ========================================================================= */

#define CSR_STR1(x) #x
#define CSR_STR(x)  CSR_STR1(x)

#define csr_read(csr)                                                          \
    ({                                                                         \
        uint64_t __v;                                                          \
        __asm__ volatile("csrr %0, " CSR_STR(csr) : "=r"(__v));                \
        __v;                                                                   \
    })

#define csr_write(csr, val)                                                    \
    do {                                                                       \
        uint64_t __v = (uint64_t)(val);                                        \
        __asm__ volatile("csrw " CSR_STR(csr) ", %0" ::"rK"(__v) : "memory");  \
    } while (0)

#define csr_set(csr, bits)                                                     \
    do {                                                                       \
        uint64_t __b = (uint64_t)(bits);                                       \
        __asm__ volatile("csrs " CSR_STR(csr) ", %0" ::"rK"(__b) : "memory");  \
    } while (0)

#define csr_clear(csr, bits)                                                   \
    do {                                                                       \
        uint64_t __b = (uint64_t)(bits);                                       \
        __asm__ volatile("csrc " CSR_STR(csr) ", %0" ::"rK"(__b) : "memory");  \
    } while (0)

#define csr_swap(csr, val)                                                     \
    ({                                                                         \
        uint64_t __v = (uint64_t)(val);                                        \
        uint64_t __r;                                                          \
        __asm__ volatile("csrrw %0, " CSR_STR(csr) ", %1"                      \
                         : "=r"(__r)                                           \
                         : "rK"(__v)                                           \
                         : "memory");                                          \
        __r;                                                                   \
    })

/* ============================================================================
 * Privilege Levels
 * ========================================================================= */

#define PRV_U 0UL
#define PRV_S 1UL
#define PRV_M 3UL

/* ============================================================================
 * mstatus / sstatus Bits
 * ========================================================================= */

#define MSTATUS_SIE   (1UL << 1)
#define MSTATUS_MIE   (1UL << 3)
#define MSTATUS_SPIE  (1UL << 5)
#define MSTATUS_MPIE  (1UL << 7)
#define MSTATUS_SPP   (1UL << 8)
#define MSTATUS_MPP_SHIFT 11
#define MSTATUS_MPP   (3UL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPP_U (0UL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPP_S (1UL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPP_M (3UL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPRV  (1UL << 17)
#define MSTATUS_SUM   (1UL << 18)
#define MSTATUS_MXR   (1UL << 19)
#define MSTATUS_TVM   (1UL << 20)
#define MSTATUS_TW    (1UL << 21)
#define MSTATUS_TSR   (1UL << 22)

/* mstatus.VS (vector state): Off / Initial / Clean / Dirty */
#define MSTATUS_VS_SHIFT 9
#define MSTATUS_VS       (3UL << MSTATUS_VS_SHIFT)
#define MSTATUS_VS_OFF   (0UL << MSTATUS_VS_SHIFT)
#define MSTATUS_VS_INITIAL (1UL << MSTATUS_VS_SHIFT)
#define MSTATUS_VS_CLEAN (2UL << MSTATUS_VS_SHIFT)
#define MSTATUS_VS_DIRTY (3UL << MSTATUS_VS_SHIFT)

/* mstatus.FS (floating-point state) */
#define MSTATUS_FS_SHIFT 13
#define MSTATUS_FS       (3UL << MSTATUS_FS_SHIFT)

#define SSTATUS_SIE   MSTATUS_SIE
#define SSTATUS_SPIE  MSTATUS_SPIE
#define SSTATUS_SPP   MSTATUS_SPP
#define SSTATUS_SUM   MSTATUS_SUM
#define SSTATUS_MXR   MSTATUS_MXR
#define SSTATUS_VS    MSTATUS_VS

/* ============================================================================
 * misa Extension Bits
 * ========================================================================= */

#define MISA_A (1UL << ('A' - 'A'))
#define MISA_C (1UL << ('C' - 'A'))
#define MISA_D (1UL << ('D' - 'A'))
#define MISA_F (1UL << ('F' - 'A'))
#define MISA_H (1UL << ('H' - 'A'))
#define MISA_I (1UL << ('I' - 'A'))
#define MISA_M (1UL << ('M' - 'A'))
#define MISA_S (1UL << ('S' - 'A'))
#define MISA_U (1UL << ('U' - 'A'))
#define MISA_V (1UL << ('V' - 'A'))

/* ============================================================================
 * Exception Causes (Synchronous)
 * ========================================================================= */

#define CAUSE_MISALIGNED_FETCH    0UL
#define CAUSE_FETCH_ACCESS        1UL
#define CAUSE_ILLEGAL_INSTRUCTION 2UL
#define CAUSE_BREAKPOINT          3UL
#define CAUSE_MISALIGNED_LOAD     4UL
#define CAUSE_LOAD_ACCESS         5UL
#define CAUSE_MISALIGNED_STORE    6UL
#define CAUSE_STORE_ACCESS        7UL
#define CAUSE_USER_ECALL          8UL
#define CAUSE_SUPERVISOR_ECALL    9UL
#define CAUSE_MACHINE_ECALL       11UL
#define CAUSE_FETCH_PAGE_FAULT    12UL
#define CAUSE_LOAD_PAGE_FAULT     13UL
#define CAUSE_STORE_PAGE_FAULT    15UL

/* ============================================================================
 * satp (Supervisor Address Translation and Protection)
 * ========================================================================= */

#define SATP_MODE_BARE 0UL
#define SATP_MODE_SV39 8UL
#define SATP_MODE_SV48 9UL
#define SATP_MODE_SHIFT 60

/* ============================================================================
 * PTE Flags (Page Table Entry for Sv39/Sv48)
 * ========================================================================= */

#define PTE_V (1UL << 0)  /* Valid */
#define PTE_R (1UL << 1)  /* Readable */
#define PTE_W (1UL << 2)  /* Writable */
#define PTE_X (1UL << 3)  /* Executable */
#define PTE_U (1UL << 4)  /* User accessible */
#define PTE_G (1UL << 5)  /* Global */
#define PTE_A (1UL << 6)  /* Accessed */
#define PTE_D (1UL << 7)  /* Dirty */

/* ============================================================================
 * PMP Configuration Bits
 * ========================================================================= */

#define PMP_R     (1U << 0)
#define PMP_W     (1U << 1)
#define PMP_X     (1U << 2)
#define PMP_A_OFF 0U
#define PMP_A_TOR (1U << 3)
#define PMP_A_NA4 (2U << 3)
#define PMP_A_NAPOT (3U << 3)
#define PMP_L     (1U << 7)

/* ============================================================================
 * Memory Barriers and Fences
 * ========================================================================= */

static inline void sfence_vma_all(void) {
    __asm__ volatile("sfence.vma x0, x0" ::: "memory");
}

static inline void fence_rw_rw(void) {
    __asm__ volatile("fence rw, rw" ::: "memory");
}

static inline void wfi(void) {
    __asm__ volatile("wfi");
}

/* ============================================================================
 * Test Environment: Assertions and Result Reporting
 * ========================================================================= */

void test_puts(const char *s);
void test_put_hex(uint64_t value);
void test_pass(void) __attribute__((noreturn));
void test_fail(const char *msg) __attribute__((noreturn));

#define TEST_ASSERT(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            test_puts("ASSERT FAIL: " #cond " @ " __FILE__ ":");               \
            test_put_hex((uint64_t)__LINE__);                                  \
            test_puts("\n");                                                   \
            test_fail("assert");                                               \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQ(a, b)                                                   \
    do {                                                                       \
        uint64_t __a = (uint64_t)(a);                                          \
        uint64_t __b = (uint64_t)(b);                                          \
        if (__a != __b) {                                                      \
            test_puts("ASSERT_EQ FAIL: " #a " != " #b " (");                   \
            test_put_hex(__a);                                                 \
            test_puts(" != ");                                                 \
            test_put_hex(__b);                                                 \
            test_puts(") @ " __FILE__ ":");                                    \
            test_put_hex((uint64_t)__LINE__);                                  \
            test_puts("\n");                                                   \
            test_fail("assert_eq");                                            \
        }                                                                      \
    } while (0)

/* ============================================================================
 * Trap Expectation and Verification
 * ========================================================================= */

void trap_init(void);
void trap_expect(uint64_t cause, uint64_t epc, uint64_t recovery_pc,
                 uint64_t tval, uint64_t tval_mask);
/* Arm the handler but do not judge cause/tval here. Use with a local
 * self-check after the trap returns (TLB/page-table tests). */
void trap_capture(void);

/*
 * After the matching trap is asserted: optionally run resolve() in M-mode,
 * then resume. Unexpected traps still fail immediately.
 *
 *   TRAP_LEAVE  — existing: S/U return to priv_enter_* caller; M skips
 *   TRAP_SKIP   — same privilege, mepc = recovery_pc or mepc+4
 *   TRAP_RETRY  — same privilege, re-execute the faulting instruction
 *
 * resolve() may fix PTEs, sfence, and arm the *next* trap_expect / trap_on.
 * Only one expect is armed at a time.
 */
#define TRAP_LEAVE 0
#define TRAP_SKIP  1
#define TRAP_RETRY 2

void trap_on(uint64_t cause, uint64_t tval, uint64_t tval_mask,
             int resume, void (*resolve)(void));
int trap_was_taken(void);
uint64_t trap_last_cause(void);
uint64_t trap_last_epc(void);
uint64_t trap_last_tval(void);
uint64_t trap_last_status(void);
uint64_t trap_last_vstart(void);
uint64_t trap_last_vl(void);
uint64_t trap_last_vtype(void);

/* Called from assembly trap entry */
void m_trap_handler(uint64_t *frame);

/* ============================================================================
 * Privilege Level Switching
 * ========================================================================= */

/* Enter S/U mode from M-mode via mret. Does not return if fn never traps back. */
void priv_enter_s(void (*fn)(void));
void priv_enter_u(void (*fn)(void));

/* Enter U-mode from S-mode via sret. Call only while already in S-mode. */
void priv_s_enter_u(void (*fn)(void));

/* Configure PMP so S/U can execute/read/write DRAM before leaving M-mode. */
void priv_allow_dram(void);

/* ============================================================================
 * PMP (Physical Memory Protection)
 * ========================================================================= */

void pmp_clear_all(void);
void pmp_set_cfg(unsigned idx, uint8_t cfg);
void pmp_set_addr(unsigned idx, uint64_t addr_raw);
void pmp_set_napot(unsigned idx, uint64_t base, uint64_t size, uint8_t cfg);
void pmp_set_tor(unsigned idx, uint64_t top, uint8_t cfg);

/* Cover [base, base+size) with one NAPOT entry; size must be power of two. */
static inline uint64_t pmp_napot_addr(uint64_t base, uint64_t size) {
    return (base >> 2) | ((size - 1UL) >> 3);
}

/* ============================================================================
 * Page Table Management (Sv39 and Sv48)
 * ========================================================================= */

#define PAGE_SIZE     4096UL
#define PAGE_SHIFT    12
#define MEGA_SIZE     (2UL * 1024UL * 1024UL)
#define GIGA_SIZE     (1UL * 1024UL * 1024UL * 1024UL)
#define TERA_SIZE     (512UL * 1024UL * 1024UL * 1024UL)

/* Page table mode */
typedef enum {
    PT_MODE_BARE = 0,
    PT_MODE_SV39 = 8,
    PT_MODE_SV48 = 9
} pt_mode_t;

/* Basic table operations */
void page_zero_table(uint64_t *table);
void page_arena_init(void);
uint64_t *page_alloc_table(void);

/* Sv39 operations (3-level, 39-bit virtual address) */
void page_map_identity_1g_sv39(uint64_t *root, uint64_t pa_base, uint64_t flags);
void page_map_4k_sv39(uint64_t *root, uint64_t va, uint64_t pa, uint64_t flags);
int page_unmap_4k_sv39(uint64_t *root, uint64_t va);
void page_enable_sv39(uint64_t *root);

/* Sv48 operations (4-level, 48-bit virtual address) */
void page_map_identity_512g_sv48(uint64_t *root, uint64_t pa_base, uint64_t flags);
void page_map_identity_1g_sv48(uint64_t *root, uint64_t pa_base, uint64_t flags);
void page_map_4k_sv48(uint64_t *root, uint64_t va, uint64_t pa, uint64_t flags);
void page_enable_sv48(uint64_t *root);

/* Generic operations */
void page_disable(void);
pt_mode_t page_get_mode(void);
void page_switch_mode(pt_mode_t new_mode, uint64_t *root);

/* Legacy compatibility (defaults to Sv39) */
static inline void page_map_identity_1g(uint64_t *root, uint64_t pa_base, uint64_t flags) {
    page_map_identity_1g_sv39(root, pa_base, flags);
}

static inline void page_map_4k(uint64_t *root, uint64_t va, uint64_t pa, uint64_t flags) {
    page_map_4k_sv39(root, va, pa, flags);
}

/* ============================================================================
 * Vector Extension (RVV) Support
 * ========================================================================= */

#ifdef EXT_V_ENABLED

/* Enable mstatus.VS = Initial so vector ops are legal. */
void vector_enable(void);

/* Force mstatus.VS = Off (vector ops should trap illegal). */
void vector_disable(void);

/* Return current VS field (0..3). */
static inline uint64_t vector_vs_state(void) {
    return (csr_read(mstatus) & MSTATUS_VS) >> MSTATUS_VS_SHIFT;
}

/* Check if V extension is available via misa.V */
static inline bool vector_available(void) {
    return (csr_read(misa) & MISA_V) != 0;
}

/* vsetvli with e32, m1 */
static inline uint64_t vector_vsetvli_e32m1(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e32, m1, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

/* vsetvli with e8, m1 */
static inline uint64_t vector_vsetvli_e8m1(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e8, m1, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

/* vsetvli with e64, m1 */
static inline uint64_t vector_vsetvli_e64m1(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e64, m1, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

static inline uint64_t vector_vsetvli_e16m1(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e16, m1, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

static inline uint64_t vector_vsetvli_e32m2(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e32, m2, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

static inline uint64_t vector_vsetvli_e32m4(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e32, m4, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

static inline uint64_t vector_vsetvli_e32m8(uint64_t avl) {
    uint64_t vl;
    __asm__ volatile("vsetvli %0, %1, e32, m8, ta, ma"
                     : "=r"(vl)
                     : "r"(avl)
                     : "memory");
    return vl;
}

#define VTYPE_VILL (1UL << 63)

static inline uint64_t vector_read_vstart(void) {
    uint64_t v;
    __asm__ volatile("csrr %0, vstart" : "=r"(v));
    return v;
}

static inline void vector_write_vstart(uint64_t v) {
    __asm__ volatile("csrw vstart, %0" ::"r"(v) : "memory");
}

/* Read vtype CSR */
static inline uint64_t vector_read_vtype(void) {
    uint64_t v;
    __asm__ volatile("csrr %0, vtype" : "=r"(v));
    return v;
}

/* Read vl CSR */
static inline uint64_t vector_read_vl(void) {
    uint64_t v;
    __asm__ volatile("csrr %0, vl" : "=r"(v));
    return v;
}

/* Vector add operation for i32 arrays */
void vector_add_i32(const int32_t *src_a, const int32_t *src_b, int32_t *dst,
                    uint64_t n);

#endif /* EXT_V_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* ARCH_RISCV_TEST_H */
