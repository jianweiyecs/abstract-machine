#include <arch/riscv-test.h>

/* Platform-defined DRAM base and size for PMP configuration */
#ifndef DRAM_BASE
#define DRAM_BASE 0x80000000UL
#endif

#ifndef DRAM_SIZE
#define DRAM_SIZE (128UL * 1024UL * 1024UL)  /* 128 MB default */
#endif

void priv_allow_dram(void) {
    pmp_clear_all();
    pmp_set_napot(0, DRAM_BASE, DRAM_SIZE, (uint8_t)(PMP_R | PMP_W | PMP_X));
}

void priv_s_enter_u(void (*fn)(void)) {
    uint64_t sstatus = csr_read(sstatus);

    sstatus &= ~SSTATUS_SPP;
    sstatus &= ~SSTATUS_SIE;
    csr_write(sstatus, sstatus);
    csr_write(sepc, (uint64_t)fn);
    __asm__ volatile("sret" ::: "memory");
}
