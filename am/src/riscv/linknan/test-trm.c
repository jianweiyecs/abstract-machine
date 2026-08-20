#include <am.h>
#include <arch/riscv-test.h>

/*
 * LinkNan / Nanhu DiffTest platform TRM
 *
 * Exit convention (XiangShan/LinkNan difftest):
 *   Execute illegal-looking NEMU trap word 0x0000006b with a0 = code.
 *   DiffTest matches (exceptionInst & 0xffff) == 0x6b and treats
 *     a0/code == 0  as HIT GOOD TRAP
 *     a0/code != 0  as abort
 *
 * UART: Nanhu-compatible base used by LinkNan platform.h (NS16550).
 * Output is best-effort; cosim pass/fail does not depend on UART.
 */

/* Nanhu/LinkNan UART (see LinkNan Spike platform.h CPU_NANHU) */
#define UART_BASE 0x40600004UL
#define UART_THR  0x0

void putch(char c) {
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    uart[UART_THR] = (uint8_t)c;
}

void halt(int code) {
    /* Keep only low 32 bits as trap code (matches common AM/NEMU convention). */
    register uint64_t a0 __asm__("a0") = (uint64_t)(unsigned)code;

    fence_rw_rw();
    /* NEMU/XS trap instruction — DiffTest turns this into HIT GOOD TRAP when a0==0 */
    __asm__ volatile(".word 0x0000006b" : : "r"(a0) : "memory");
    for (;;)
        __asm__ volatile("wfi" ::: "memory");
}
