#include <arch/riscv-test.h>

#if HART_MAX > 1
static volatile unsigned seen[HART_MAX];

static void worker(void) {
    unsigned id = hart_id();
    TEST_ASSERT(id > 0 && id < HART_MAX);
    TEST_ASSERT_EQ(hart_physical_id(), HART_BASE + id);
    seen[id]++;
}

#endif

int main(void) {
    TEST_ASSERT_EQ(hart_id(), 0);
    TEST_ASSERT_EQ(hart_count(), HART_MAX);
    TEST_ASSERT_EQ(hart_physical_id(), HART_BASE);
#if HART_MAX > 1
    for (unsigned round = 1; round <= 3; round++) {
        for (unsigned id = 1; id < HART_MAX; id++)
            hart_start(id, worker);
        for (unsigned id = 1; id < HART_MAX; id++) {
            hart_join(id);
            TEST_ASSERT_EQ(seen[id], round);
        }
    }
    hart_start(1, worker);
    hart_start(1, worker);
    hart_join(1);
    TEST_ASSERT_EQ(seen[1], 5);
#endif
    test_pass();
}
