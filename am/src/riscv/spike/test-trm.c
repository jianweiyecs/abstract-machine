#include <am.h>
#include <arch/riscv-test.h>

/* Platform constants for Spike */
#define UART_BASE 0x10000000UL
#define UART_THR  0x0

/* HTIF exit mechanism for Spike */
volatile uint64_t tohost __attribute__((section(".tohost"), aligned(8))) = 0;
volatile uint64_t fromhost __attribute__((section(".tohost"), aligned(8))) = 0;

/* Override AM's putch for test output */
void putch(char c) {
    /* Fire-and-forget UART write (no LSR polling to avoid hangs) */
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    uart[UART_THR] = (uint8_t)c;
}

/* Override AM's halt for test exit via HTIF tohost.
 *
 * Spike polls the tohost symbol from the host side. After writing the exit
 * code we keep rewriting tohost (in case the host clears the slot) and
 * idle with wfi so the simulator can make progress.
 */
void halt(int code) {
    uint64_t payload = ((uint64_t)(unsigned)code << 1) | 1UL;
    fence_rw_rw();
    for (;;) {
        tohost = payload;
        fence_rw_rw();
        __asm__ volatile("wfi" ::: "memory");
    }
}
