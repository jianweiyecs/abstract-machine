#include <arch/riscv-test.h>

void pmp_set_addr(unsigned idx, uint64_t addr_raw) {
    switch (idx) {
    case 0: csr_write(pmpaddr0, addr_raw); break;
    case 1: csr_write(pmpaddr1, addr_raw); break;
    case 2: csr_write(pmpaddr2, addr_raw); break;
    case 3: csr_write(pmpaddr3, addr_raw); break;
    case 4: csr_write(pmpaddr4, addr_raw); break;
    case 5: csr_write(pmpaddr5, addr_raw); break;
    case 6: csr_write(pmpaddr6, addr_raw); break;
    case 7: csr_write(pmpaddr7, addr_raw); break;
    case 8: csr_write(pmpaddr8, addr_raw); break;
    case 9: csr_write(pmpaddr9, addr_raw); break;
    case 10: csr_write(pmpaddr10, addr_raw); break;
    case 11: csr_write(pmpaddr11, addr_raw); break;
    case 12: csr_write(pmpaddr12, addr_raw); break;
    case 13: csr_write(pmpaddr13, addr_raw); break;
    case 14: csr_write(pmpaddr14, addr_raw); break;
    case 15: csr_write(pmpaddr15, addr_raw); break;
    default:
        test_fail("pmp idx");
    }
}

void pmp_set_cfg(unsigned idx, uint8_t cfg) {
    unsigned reg = idx >> 3; /* 0 -> pmpcfg0, 1 -> pmpcfg2 on RV64 */
    unsigned off = (idx & 7) * 8;
    uint64_t val;
    uint64_t mask = 0xffUL << off;

    if (reg == 0)
        val = csr_read(pmpcfg0);
    else if (reg == 1)
        val = csr_read(pmpcfg2);
    else
        test_fail("pmp cfg reg");

    val = (val & ~mask) | ((uint64_t)cfg << off);

    if (reg == 0)
        csr_write(pmpcfg0, val);
    else
        csr_write(pmpcfg2, val);
}

void pmp_clear_all(void) {
    unsigned i;

    csr_write(pmpcfg0, 0);
    csr_write(pmpcfg2, 0);
    for (i = 0; i < 16; i++)
        pmp_set_addr(i, 0);
}

void pmp_set_napot(unsigned idx, uint64_t base, uint64_t size, uint8_t cfg) {
    if (size < 8 || (size & (size - 1)) != 0)
        test_fail("pmp napot size");
    if ((base & (size - 1)) != 0)
        test_fail("pmp napot align");

    pmp_set_addr(idx, pmp_napot_addr(base, size));
    pmp_set_cfg(idx, (uint8_t)((cfg & (PMP_R | PMP_W | PMP_X | PMP_L)) | PMP_A_NAPOT));
}

void pmp_set_tor(unsigned idx, uint64_t top, uint8_t cfg) {
    pmp_set_addr(idx, top >> 2);
    pmp_set_cfg(idx, (uint8_t)((cfg & (PMP_R | PMP_W | PMP_X | PMP_L)) | PMP_A_TOR));
}
