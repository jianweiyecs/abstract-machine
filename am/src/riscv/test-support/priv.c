#include <arch/riscv-test.h>

void priv_allow_dram(void) {
    pmp_clear_all();
    if (DRAM_SIZE >= 8 && (DRAM_SIZE & (DRAM_SIZE - 1)) == 0 &&
        (DRAM_BASE & (DRAM_SIZE - 1)) == 0) {
        pmp_set_napot(0, DRAM_BASE, DRAM_SIZE, (uint8_t)(PMP_R | PMP_W | PMP_X));
    } else {
        pmp_set_addr(0, (uint64_t)DRAM_BASE >> 2);
        pmp_set_tor(1, (uint64_t)DRAM_BASE + DRAM_SIZE, (uint8_t)(PMP_R | PMP_W | PMP_X));
    }
}

void priv_s_enter_u(void (*fn)(void)) {
    uint64_t sstatus = csr_read(sstatus);

    sstatus &= ~SSTATUS_SPP;
    sstatus &= ~SSTATUS_SIE;
    csr_write(sstatus, sstatus);
    csr_write(sepc, (uint64_t)fn);
    __asm__ volatile("sret" ::: "memory");
}
