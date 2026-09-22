#include <arch/riscv-test.h>

/*
 * Mailbox lives in .data (explicit zeros) so hart-0 BSS clear cannot
 * wipe it while secondaries already spin on boot_ready.
 */
#define HART_DATA __attribute__((section(".data"), aligned(8)))

volatile uint64_t hart_boot_ready HART_DATA = 0;
void (*volatile hart_mailbox[HART_MAX])(void) HART_DATA = { 0 };
volatile uint64_t hart_done[HART_MAX] HART_DATA = { 0 };
hart_spinlock_t hart_uart_lock HART_DATA = { 0 };

/* Owned by hart 0; completion is acquired through hart_done before reuse. */
static unsigned char hart_pending[HART_MAX];
static volatile uint32_t hart_bar_count HART_DATA = 0;
static volatile uint32_t hart_bar_sense HART_DATA = 0;

unsigned hart_count(void)
{
    return HART_MAX;
}

void hart_start(unsigned id, void (*fn)(void))
{
    if (hart_id() != 0)
        test_fail("hart_start from non-0");
    TEST_REQUIRE(id < HART_MAX, "hart_start needs more configured harts");
    if (id == 0 || !fn)
        test_fail("hart_start id");

    if (hart_pending[id])
        hart_join(id);
    hart_done[id] = 0;
    hart_pending[id] = 1;
    fence_rw_rw();
    hart_mailbox[id] = fn;
    fence_rw_rw();
}

void hart_join(unsigned id)
{
    if (hart_id() != 0)
        test_fail("hart_join from non-0");
    if (id == 0 || id >= HART_MAX)
        test_fail("hart_join id");
    if (!hart_pending[id])
        test_fail("hart_join without a task");
    while (!hart_done[id])
        ;
    fence_rw_rw();
    hart_pending[id] = 0;
}

void hart_check_complete(void)
{
    for (unsigned id = 1; id < HART_MAX; id++)
        if (hart_pending[id])
            test_fail("worker task was not joined");
}

void hart_barrier(unsigned n)
{
#if HART_MAX == 1
    if (n != 1)
        test_skip("barrier needs more configured harts");
    fence_rw_rw();
#else
    uint32_t mysense;
    uint32_t old;
    uint32_t one = 1;

    if (n == 0 || n > HART_MAX)
        test_fail("hart_barrier n");

    mysense = hart_bar_sense;
    /* Publish pre-barrier writes and acquire preceding arrivals before the
     * last hart releases the sense. The final fence acquires that release. */
    __asm__ volatile(
        ".option push\n.option norvc\n"
        "amoadd.w.aqrl %0, %1, (%2)\n"
        ".option pop\n"
        : "=r"(old)
        : "r"(one), "r"(&hart_bar_count)
        : "memory");
    if (old + 1 == n) {
        hart_bar_count = 0;
        fence_rw_rw();
        hart_bar_sense = mysense + 1;
    } else {
        while (hart_bar_sense == mysense)
            ;
    }
    fence_rw_rw();
#endif
}

void hart_spin_lock(hart_spinlock_t *lock)
{
#if HART_MAX == 1
    (void)lock;
#else
    uint32_t got;
    uint32_t one = 1;

    do {
        __asm__ volatile(
            ".option push\n.option norvc\n"
            "amoswap.w %0, %1, (%2)\n"
            ".option pop\n"
            : "=r"(got)
            : "r"(one), "r"(&lock->lock)
            : "memory");
    } while (got);
    fence_rw_rw();
#endif
}

void hart_spin_unlock(hart_spinlock_t *lock)
{
    fence_rw_rw();
    lock->lock = 0;
}

void clint_msip_set(unsigned hart)
{
    volatile uint32_t *msip;

    TEST_REQUIRE(CLINT_BASE != 0, "platform has no CLINT MSIP");
    if (hart >= HART_MAX)
        test_fail("clint hart");
    msip = (volatile uint32_t *)(uintptr_t)(CLINT_BASE + 4UL * (HART_BASE + (unsigned long)hart));
    *msip = 1;
    fence_rw_rw();
}

void clint_msip_clear(unsigned hart)
{
    volatile uint32_t *msip;

    TEST_REQUIRE(CLINT_BASE != 0, "platform has no CLINT MSIP");
    if (hart >= HART_MAX)
        test_fail("clint hart");
    msip = (volatile uint32_t *)(uintptr_t)(CLINT_BASE + 4UL * (HART_BASE + (unsigned long)hart));
    *msip = 0;
    fence_rw_rw();
}
