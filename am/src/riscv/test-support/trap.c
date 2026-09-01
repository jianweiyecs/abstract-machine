#include <arch/riscv-test.h>

/* Defined in priv_entry.S — resume M-mode C caller of priv_enter_*. */
extern void priv_resume_m(void);

struct trap_expect_state {
    int armed;
    int taken;
    int check;
    int resume;
    void (*resolve)(void);
    uint64_t cause;
    uint64_t epc;
    uint64_t recovery_pc;
    uint64_t tval;
    uint64_t tval_mask;
    uint64_t last_cause;
    uint64_t last_epc;
    uint64_t last_tval;
    uint64_t last_status;
    uint64_t last_vstart;
    uint64_t last_vl;
    uint64_t last_vtype;
};

static struct trap_expect_state g_expect;

/*
 * Bytes to skip past the insn at mepc. Never hardcode +4.
 *
 * Prefer mtval on illegal (spec: tval is the faulting insn encoding when
 * implemented). Fetch/PF/misaligned-fetch must not load from mepc — that
 * address may not be a valid data access.
 *
 * .word 0 is a 32-bit illegal (two zero halfwords). mtval==0 ⇒ 4, not
 * c.unimp. Real 16-bit C (c.ebreak=0x9002) has bits[1:0]!=11 and mtval!=0
 * when tval reports the insn.
 */
static uint64_t trap_insn_len(uint64_t mepc, uint64_t mcause, uint64_t mtval)
{
    if (mcause == CAUSE_ILLEGAL_INSTRUCTION) {
        if (mtval == 0)
            return 4;
        return ((mtval & 0x3) == 0x3) ? 4 : 2;
    }
    if (mcause == CAUSE_MISALIGNED_FETCH || mcause == CAUSE_FETCH_ACCESS ||
        mcause == CAUSE_FETCH_PAGE_FAULT)
        return 4;
    {
        uint16_t half = *(volatile uint16_t *)(uintptr_t)mepc;
        return ((half & 0x3) == 0x3) ? 4 : 2;
    }
}

void trap_init(void) {
    g_expect.armed = 0;
    g_expect.taken = 0;
    g_expect.check = 0;
    g_expect.resume = TRAP_LEAVE;
    g_expect.resolve = 0;
}

void trap_expect(uint64_t cause, uint64_t epc, uint64_t recovery_pc,
                 uint64_t tval, uint64_t tval_mask) {
    g_expect.armed = 1;
    g_expect.taken = 0;
    g_expect.check = 1;
    g_expect.resume = TRAP_LEAVE;
    g_expect.resolve = 0;
    g_expect.cause = cause;
    g_expect.epc = epc;
    g_expect.recovery_pc = recovery_pc;
    g_expect.tval = tval;
    g_expect.tval_mask = tval_mask;
}

void trap_capture(void) {
    g_expect.armed = 1;
    g_expect.taken = 0;
    g_expect.check = 0;
    g_expect.resume = TRAP_LEAVE;
    g_expect.resolve = 0;
    g_expect.cause = 0;
    g_expect.epc = 0;
    g_expect.recovery_pc = 0;
    g_expect.tval = 0;
    g_expect.tval_mask = 0;
}

void trap_on(uint64_t cause, uint64_t tval, uint64_t tval_mask,
             int resume, void (*resolve)(void)) {
    g_expect.armed = 1;
    g_expect.taken = 0;
    g_expect.check = 1;
    g_expect.resume = resume;
    g_expect.resolve = resolve;
    g_expect.cause = cause;
    g_expect.epc = 0;
    g_expect.recovery_pc = 0;
    g_expect.tval = tval;
    g_expect.tval_mask = tval_mask;
}

int trap_was_taken(void) {
    return g_expect.taken;
}

uint64_t trap_last_cause(void) {
    return g_expect.last_cause;
}

uint64_t trap_last_epc(void) {
    return g_expect.last_epc;
}

uint64_t trap_last_tval(void) {
    return g_expect.last_tval;
}

uint64_t trap_last_status(void) {
    return g_expect.last_status;
}

uint64_t trap_last_vstart(void) {
    return g_expect.last_vstart;
}

uint64_t trap_last_vl(void) {
    return g_expect.last_vl;
}

uint64_t trap_last_vtype(void) {
    return g_expect.last_vtype;
}

void m_trap_handler(uint64_t *frame) {
    uint64_t mepc = frame[32];
    uint64_t mstatus = frame[33];
    uint64_t mcause = frame[34];
    uint64_t mtval = frame[35];
    uint64_t prev;

    g_expect.last_cause = mcause;
    g_expect.last_epc = mepc;
    g_expect.last_tval = mtval;
    g_expect.last_status = mstatus;
#ifdef EXT_V_ENABLED
    /* Snapshot before the handler executes any code that could alter V CSRs. */
    if ((mstatus & MSTATUS_VS) != MSTATUS_VS_OFF) {
        g_expect.last_vstart = csr_read(vstart);
        g_expect.last_vl = csr_read(vl);
        g_expect.last_vtype = csr_read(vtype);
    } else {
        g_expect.last_vstart = UINT64_MAX;
        g_expect.last_vl = UINT64_MAX;
        g_expect.last_vtype = UINT64_MAX;
    }
#else
    g_expect.last_vstart = UINT64_MAX;
    g_expect.last_vl = UINT64_MAX;
    g_expect.last_vtype = UINT64_MAX;
#endif

    if (!g_expect.armed) {
        test_puts("UNEXPECTED TRAP cause=");
        test_put_hex(mcause);
        test_puts(" epc=");
        test_put_hex(mepc);
        test_puts(" tval=");
        test_put_hex(mtval);
        test_puts("\n");
        test_fail("unexpected trap");
    }

    if (g_expect.check) {
        if (mcause != g_expect.cause) {
            test_puts("TRAP CAUSE MISMATCH got=");
            test_put_hex(mcause);
            test_puts(" want=");
            test_put_hex(g_expect.cause);
            test_puts("\n");
            test_fail("trap cause");
        }

        if (g_expect.epc != 0 && mepc != g_expect.epc) {
            test_puts("TRAP EPC MISMATCH got=");
            test_put_hex(mepc);
            test_puts(" want=");
            test_put_hex(g_expect.epc);
            test_puts("\n");
            test_fail("trap epc");
        }

        if ((mtval & g_expect.tval_mask) != (g_expect.tval & g_expect.tval_mask)) {
            test_puts("TRAP TVAL MISMATCH got=");
            test_put_hex(mtval);
            test_puts(" want=");
            test_put_hex(g_expect.tval);
            test_puts(" mask=");
            test_put_hex(g_expect.tval_mask);
            test_puts("\n");
            test_fail("trap tval");
        }
    }

    {
        int how = g_expect.resume;
        void (*resolve)(void) = g_expect.resolve;
        /*
         * Default skip is the faulting insn length, not a hardcoded +4.
         * C/Zca: bits[1:0] != 11 → 16-bit (e.g. c.ebreak = 0x9002).
         * +4 on a 2-byte insn lands in the middle of the next 32-bit insn
         * and the leftover half-word traps as illegal (cause=2, tval=0).
         */
        uint64_t skip_pc = g_expect.recovery_pc ? g_expect.recovery_pc
                                                : (mepc + trap_insn_len(mepc, mcause, mtval));

        g_expect.armed = 0;
        g_expect.taken = 1;
        g_expect.resume = TRAP_LEAVE;
        g_expect.resolve = 0;

        /* May arm the *next* trap_expect / trap_on. Must not consume skip_pc. */
        if (resolve)
            resolve();

        prev = (mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;

        if (how == TRAP_RETRY || how == TRAP_SKIP) {
            /*
             * Stay in the faulting privilege. Pick up mstatus bits that
             * resolve() may have changed (VS/FS), but keep MPP from the trap.
             */
            uint64_t now = csr_read(mstatus);

            now &= ~MSTATUS_MPP;
            now |= (mstatus & MSTATUS_MPP);
            now &= ~MSTATUS_MPRV;
            frame[32] = (how == TRAP_RETRY) ? mepc : skip_pc;
            frame[33] = now;
            return;
        }

        /*
         * TRAP_LEAVE (default, existing tests):
         * S/U → priv_enter_* caller via priv_resume_m.
         * M → mepc+ilen or explicit recovery_pc.
         */
        if (prev != PRV_M) {
            frame[32] = (uint64_t)(uintptr_t)priv_resume_m;
        } else if (g_expect.recovery_pc != 0) {
            frame[32] = g_expect.recovery_pc;
        } else {
            frame[32] = mepc + trap_insn_len(mepc, mcause, mtval);
        }

        mstatus &= ~MSTATUS_MPP;
        mstatus |= MSTATUS_MPP_M;
        mstatus &= ~MSTATUS_MPRV;
        frame[33] = mstatus;
    }
}
