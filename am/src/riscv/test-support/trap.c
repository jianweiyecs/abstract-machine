#include <arch/riscv-test.h>

/* Defined in priv_entry.S — resume M-mode C caller of priv_enter_*. */
extern void priv_resume_m(void);

struct trap_expect_state {
    uint64_t *active_frame;
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

static struct trap_expect_state g_expect[HART_MAX];

static struct trap_expect_state *expect(void)
{
    unsigned long id = hart_id();
    if (id >= (unsigned long)HART_MAX)
        test_fail("trap hart");
    return &g_expect[id];
}

/*
 * Bytes to skip past the insn at mepc. Never hardcode +4.
 *
 * A nonzero illegal-instruction mtval contains instruction bits. Zero
 * may mean c.unimp or unavailable bits; read the first halfword instead.
 * MPRV uses the trapped privilege's address space; MXR permits X-only PTEs.
 * Code must be data-readable through PMP. Unreadable/faulting instruction
 * addresses require an explicit recovery PC (trap_expect / trap_skip_to).
 */
static uint64_t trap_insn_len(uint64_t mepc, uint64_t mcause, uint64_t mtval,
                              uint64_t status)
{
    uint64_t saved, half, access_status;

    if (mcause == CAUSE_ILLEGAL_INSTRUCTION && mtval != 0)
        return ((mtval & 0x3) == 0x3) ? 4 : 2;
    if (mcause == CAUSE_USER_ECALL || mcause == CAUSE_SUPERVISOR_ECALL ||
        mcause == CAUSE_MACHINE_ECALL)
        return 4;
    if (mcause == CAUSE_MISALIGNED_FETCH || mcause == CAUSE_FETCH_ACCESS ||
        mcause == CAUSE_FETCH_PAGE_FAULT)
        test_fail("instruction fetch fault needs explicit recovery PC");

    access_status = (csr_read(mstatus) & ~MSTATUS_MPP) |
                    (status & MSTATUS_MPP) | MSTATUS_MPRV | MSTATUS_MXR;
    /* No C memory access (including stack spills) while MPRV is active. */
    __asm__ volatile(
        "csrrw %0, mstatus, %2\n"
        "lhu %1, 0(%3)\n"
        "csrw mstatus, %0\n"
        : "=&r"(saved), "=&r"(half)
        : "r"(access_status), "r"(mepc)
        : "memory");
    return ((half & 0x3) == 0x3) ? 4 : 2;
}

void trap_init(void) {
    unsigned i;

    for (i = 0; i < HART_MAX; i++) {
        g_expect[i].armed = 0;
        g_expect[i].taken = 0;
        g_expect[i].check = 0;
        g_expect[i].resume = TRAP_LEAVE;
        g_expect[i].resolve = 0;
    }
}

#ifdef __riscv_flen
void trap_commit_fcsr(void) {
    uint64_t *frame = expect()->active_frame;

    if (!frame || !(csr_read(mstatus) & MSTATUS_FS))
        test_fail("FCSR commit requires an active resolve and FS enabled");
    frame[36] = csr_read(fcsr);
}
#endif

#ifdef EXT_V_ENABLED
void trap_commit_vector_csrs(void) {
    uint64_t *frame = expect()->active_frame;

    if (!frame || !(csr_read(mstatus) & MSTATUS_VS))
        test_fail("vector CSR commit requires an active resolve and VS enabled");
    frame[39] = csr_read(vl);
    frame[40] = csr_read(vtype);
    frame[41] = csr_read(vstart);
    frame[42] = csr_read(vcsr);
}
#endif

void trap_cancel(void) {
    expect()->armed = 0;
}

void trap_check_complete(void) {
    for (unsigned i = 0; i < HART_MAX; i++)
        if (g_expect[i].armed)
            test_fail("expected trap was not taken");
}

void trap_expect(uint64_t cause, uint64_t epc, uint64_t recovery_pc,
                 uint64_t tval, uint64_t tval_mask) {
    struct trap_expect_state *e = expect();

    if (e->armed)
        test_fail("overwriting an unconsumed trap expectation");
    e->armed = 1;
    e->taken = 0;
    e->check = 1;
    e->resume = TRAP_LEAVE;
    e->resolve = 0;
    e->cause = cause;
    e->epc = epc;
    e->recovery_pc = recovery_pc;
    e->tval = tval;
    e->tval_mask = tval_mask;
}

void trap_capture(void) {
    struct trap_expect_state *e = expect();

    if (e->armed)
        test_fail("overwriting an unconsumed trap expectation");
    e->armed = 1;
    e->taken = 0;
    e->check = 0;
    e->resume = TRAP_LEAVE;
    e->resolve = 0;
    e->cause = 0;
    e->epc = 0;
    e->recovery_pc = 0;
    e->tval = 0;
    e->tval_mask = 0;
}

void trap_on(uint64_t cause, uint64_t tval, uint64_t tval_mask,
             int resume, void (*resolve)(void)) {
    struct trap_expect_state *e = expect();

    if (e->armed)
        test_fail("overwriting an unconsumed trap expectation");
    e->armed = 1;
    e->taken = 0;
    e->check = 1;
    e->resume = resume;
    e->resolve = resolve;
    e->cause = cause;
    e->epc = 0;
    e->recovery_pc = 0;
    e->tval = tval;
    e->tval_mask = tval_mask;
}

void trap_skip_to(uint64_t recovery_pc) {
    struct trap_expect_state *e = expect();

    if (!e->armed || e->resume != TRAP_SKIP || recovery_pc == 0)
        test_fail("trap_skip_to requires armed TRAP_SKIP and nonzero PC");
    e->recovery_pc = recovery_pc;
}

int trap_was_taken(void) {
    return expect()->taken;
}

uint64_t trap_last_cause(void) {
    return expect()->last_cause;
}

uint64_t trap_last_epc(void) {
    return expect()->last_epc;
}

uint64_t trap_last_tval(void) {
    return expect()->last_tval;
}

uint64_t trap_last_status(void) {
    return expect()->last_status;
}

uint64_t trap_last_vstart(void) {
    return expect()->last_vstart;
}

uint64_t trap_last_vl(void) {
    return expect()->last_vl;
}

uint64_t trap_last_vtype(void) {
    return expect()->last_vtype;
}

void m_trap_handler(uint64_t *frame) {
    uint64_t mepc = frame[32];
    uint64_t mstatus = frame[33];
    uint64_t mcause = frame[34];
    uint64_t mtval = frame[35];
    uint64_t prev;
    struct trap_expect_state *e = expect();

    e->last_cause = mcause;
    e->last_epc = mepc;
    e->last_tval = mtval;
    e->last_status = mstatus;
#ifdef EXT_V_ENABLED
    /* Assembly snapshots these before saving vector registers. */
    if ((mstatus & MSTATUS_VS) != MSTATUS_VS_OFF) {
        e->last_vstart = frame[41];
        e->last_vl = frame[39];
        e->last_vtype = frame[40];
    } else {
        e->last_vstart = UINT64_MAX;
        e->last_vl = UINT64_MAX;
        e->last_vtype = UINT64_MAX;
    }
#else
    e->last_vstart = UINT64_MAX;
    e->last_vl = UINT64_MAX;
    e->last_vtype = UINT64_MAX;
#endif

    if (!e->armed) {
        test_puts("UNEXPECTED TRAP cause=");
        test_put_hex(mcause);
        test_puts(" epc=");
        test_put_hex(mepc);
        test_puts(" tval=");
        test_put_hex(mtval);
        test_puts("\n");
        test_fail("unexpected trap");
    }

    if (e->check) {
        if (mcause != e->cause) {
            test_puts("TRAP CAUSE MISMATCH got=");
            test_put_hex(mcause);
            test_puts(" want=");
            test_put_hex(e->cause);
            test_puts("\n");
            test_fail("trap cause");
        }

        if (e->epc != 0 && mepc != e->epc) {
            test_puts("TRAP EPC MISMATCH got=");
            test_put_hex(mepc);
            test_puts(" want=");
            test_put_hex(e->epc);
            test_puts("\n");
            test_fail("trap epc");
        }

        if ((mtval & e->tval_mask) != (e->tval & e->tval_mask)) {
            test_puts("TRAP TVAL MISMATCH got=");
            test_put_hex(mtval);
            test_puts(" want=");
            test_put_hex(e->tval);
            test_puts(" mask=");
            test_put_hex(e->tval_mask);
            test_puts("\n");
            test_fail("trap tval");
        }
    }

    {
        int how = e->resume;
        void (*resolve)(void) = e->resolve;
        uint64_t recovery = e->recovery_pc;
        uint64_t skip_pc = recovery;

        prev = (mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
        /* RETRY and S/U LEAVE never inspect the instruction. Decode before
         * resolve(), which may replace its mapping or arm the next trap. */
        if (!skip_pc && (how == TRAP_SKIP ||
                        (how == TRAP_LEAVE && prev == PRV_M)))
            skip_pc = mepc + trap_insn_len(mepc, mcause, mtval, mstatus);

        e->armed = 0;
        e->taken = 1;
        e->resume = TRAP_LEAVE;
        e->resolve = 0;

        /* May arm the *next* trap_expect / trap_on. Must not consume skip_pc. */
        e->active_frame = frame;
        if (resolve)
            resolve();
        e->active_frame = 0;

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
        } else {
            frame[32] = skip_pc;
        }

        mstatus &= ~MSTATUS_MPP;
        mstatus |= MSTATUS_MPP_M;
        mstatus &= ~MSTATUS_MPRV;
        frame[33] = mstatus;
    }
}
