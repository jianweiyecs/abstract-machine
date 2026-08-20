#include <arch/riscv-test.h>
#include <am.h>

/* ANSI color codes */
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_RESET  "\033[0m"

void test_puts(const char *s) {
    if (!s)
        return;
    while (*s)
        putch(*s++);
}

void test_put_hex(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    int i;

    test_puts("0x");
    for (i = 60; i >= 0; i -= 4)
        putch(digits[(value >> i) & 0xf]);
}

void test_pass(void) {
    test_puts(COLOR_GREEN "PASS" COLOR_RESET "\n");
    halt(0);
}

void test_fail(const char *msg) {
    test_puts(COLOR_RED "FAIL" COLOR_RESET);
    if (msg) {
        test_puts(": ");
        test_puts(msg);
    }
    test_puts("\n");
    halt(1);
}
