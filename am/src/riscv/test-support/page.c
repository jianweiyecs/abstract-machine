#include <arch/riscv-test.h>

extern char __page_arena_start[];
extern char __page_arena_end[];

static uint8_t *arena_ptr;
static uint8_t *arena_end;

void page_arena_init(void) {
    arena_ptr = (uint8_t *)__page_arena_start;
    arena_end = (uint8_t *)__page_arena_end;
}

uint64_t *page_alloc_table(void) {
    uint64_t *table;
    uintptr_t p;

    if (!arena_ptr)
        page_arena_init();

    p = ((uintptr_t)arena_ptr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (p + PAGE_SIZE > (uintptr_t)arena_end)
        test_fail("page arena OOM");

    table = (uint64_t *)p;
    arena_ptr = (uint8_t *)(p + PAGE_SIZE);
    page_zero_table(table);
    return table;
}

void page_zero_table(uint64_t *table) {
    unsigned i;

    for (i = 0; i < 512; i++)
        table[i] = 0;
}

static uint64_t pte_leaf(uint64_t pa, uint64_t flags) {
    return ((pa >> 12) << 10) | flags | PTE_V;
}

static uint64_t pte_next(uint64_t *next) {
    return (((uint64_t)(uintptr_t)next >> 12) << 10) | PTE_V;
}

static int is_leaf(uint64_t pte) {
    return (pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X));
}

/* ============================================================================
 * Sv39 Implementation (3-level page table, 39-bit VA)
 * ========================================================================= */

void page_map_identity_1g_sv39(uint64_t *root, uint64_t pa_base, uint64_t flags) {
    unsigned idx;

    if ((pa_base & (GIGA_SIZE - 1)) != 0)
        test_fail("1g align");

    idx = (pa_base >> 30) & 0x1ff;
    root[idx] = pte_leaf(pa_base, flags | PTE_A | PTE_D);
}

void page_map_4k_sv39(uint64_t *root, uint64_t va, uint64_t pa, uint64_t flags) {
    unsigned vpn2 = (va >> 30) & 0x1ff;
    unsigned vpn1 = (va >> 21) & 0x1ff;
    unsigned vpn0 = (va >> 12) & 0x1ff;
    uint64_t *l1;
    uint64_t *l0;
    unsigned i;

    if (!(root[vpn2] & PTE_V)) {
        l1 = page_alloc_table();
        root[vpn2] = pte_next(l1);
    } else if (is_leaf(root[vpn2])) {
        /* Demote 1G leaf -> 512 x 2M identity leaves. */
        uint64_t base = (root[vpn2] >> 10) << 12;
        uint64_t leaf_flags = root[vpn2] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                            PTE_G | PTE_A | PTE_D);

        l1 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l1[i] = pte_leaf(base + (uint64_t)i * MEGA_SIZE, leaf_flags);
        root[vpn2] = pte_next(l1);
    } else {
        l1 = (uint64_t *)((root[vpn2] >> 10) << 12);
    }

    if (!(l1[vpn1] & PTE_V)) {
        l0 = page_alloc_table();
        l1[vpn1] = pte_next(l0);
    } else if (is_leaf(l1[vpn1])) {
        /* Demote 2M leaf -> 512 x 4K identity leaves. */
        uint64_t base = (l1[vpn1] >> 10) << 12;
        uint64_t leaf_flags = l1[vpn1] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                          PTE_G | PTE_A | PTE_D);

        l0 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l0[i] = pte_leaf(base + (uint64_t)i * PAGE_SIZE, leaf_flags);
        l1[vpn1] = pte_next(l0);
    } else {
        l0 = (uint64_t *)((l1[vpn1] >> 10) << 12);
    }

    l0[vpn0] = pte_leaf(pa, flags | PTE_A | PTE_D);
}

int page_unmap_4k_sv39(uint64_t *root, uint64_t va) {
    unsigned vpn2 = (va >> 30) & 0x1ff;
    unsigned vpn1 = (va >> 21) & 0x1ff;
    unsigned vpn0 = (va >> 12) & 0x1ff;
    uint64_t pte;
    uint64_t *l1;
    uint64_t *l0;
    unsigned i;

    if ((va & (PAGE_SIZE - 1)) != 0)
        test_fail("4k unmap align");

    pte = root[vpn2];
    if (!(pte & PTE_V))
        return 0;
    if (is_leaf(pte)) {
        uint64_t base = (pte >> 10) << 12;
        uint64_t leaf_flags = pte & (PTE_R | PTE_W | PTE_X | PTE_U |
                                     PTE_G | PTE_A | PTE_D);

        l1 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l1[i] = pte_leaf(base + (uint64_t)i * MEGA_SIZE, leaf_flags);
        root[vpn2] = pte_next(l1);
    } else {
        l1 = (uint64_t *)((pte >> 10) << 12);
    }

    pte = l1[vpn1];
    if (!(pte & PTE_V))
        return 0;
    if (is_leaf(pte)) {
        uint64_t base = (pte >> 10) << 12;
        uint64_t leaf_flags = pte & (PTE_R | PTE_W | PTE_X | PTE_U |
                                     PTE_G | PTE_A | PTE_D);

        l0 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l0[i] = pte_leaf(base + (uint64_t)i * PAGE_SIZE, leaf_flags);
        l1[vpn1] = pte_next(l0);
    } else {
        l0 = (uint64_t *)((pte >> 10) << 12);
    }

    if (!(l0[vpn0] & PTE_V))
        return 0;
    l0[vpn0] = 0;
    sfence_vma_all();
    return 1;
}

void page_enable_sv39(uint64_t *root) {
    uint64_t ppn = ((uint64_t)(uintptr_t)root) >> 12;
    uint64_t satp = (SATP_MODE_SV39 << SATP_MODE_SHIFT) | ppn;

    csr_write(satp, satp);
    sfence_vma_all();
}

/* ============================================================================
 * Sv48 Implementation (4-level page table, 48-bit VA)
 * ========================================================================= */

void page_map_identity_512g_sv48(uint64_t *root, uint64_t pa_base, uint64_t flags) {
    unsigned idx;

    if ((pa_base & (TERA_SIZE - 1)) != 0)
        test_fail("512g align");

    idx = (pa_base >> 39) & 0x1ff;
    root[idx] = pte_leaf(pa_base, flags | PTE_A | PTE_D);
}

void page_map_identity_1g_sv48(uint64_t *root, uint64_t pa_base, uint64_t flags) {
    unsigned vpn3 = (pa_base >> 39) & 0x1ff;
    unsigned vpn2 = (pa_base >> 30) & 0x1ff;
    uint64_t *l2;
    unsigned i;

    if ((pa_base & (GIGA_SIZE - 1)) != 0)
        test_fail("1g align");

    if (!(root[vpn3] & PTE_V)) {
        l2 = page_alloc_table();
        root[vpn3] = pte_next(l2);
    } else if (is_leaf(root[vpn3])) {
        /* Demote 512G leaf -> 512 x 1G identity leaves. */
        uint64_t base = (root[vpn3] >> 10) << 12;
        uint64_t leaf_flags = root[vpn3] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                            PTE_G | PTE_A | PTE_D);

        l2 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l2[i] = pte_leaf(base + (uint64_t)i * GIGA_SIZE, leaf_flags);
        root[vpn3] = pte_next(l2);
    } else {
        l2 = (uint64_t *)((root[vpn3] >> 10) << 12);
    }

    l2[vpn2] = pte_leaf(pa_base, flags | PTE_A | PTE_D);
}

void page_map_4k_sv48(uint64_t *root, uint64_t va, uint64_t pa, uint64_t flags) {
    unsigned vpn3 = (va >> 39) & 0x1ff;
    unsigned vpn2 = (va >> 30) & 0x1ff;
    unsigned vpn1 = (va >> 21) & 0x1ff;
    unsigned vpn0 = (va >> 12) & 0x1ff;
    uint64_t *l2, *l1, *l0;
    unsigned i;

    /* Level 3 -> Level 2 */
    if (!(root[vpn3] & PTE_V)) {
        l2 = page_alloc_table();
        root[vpn3] = pte_next(l2);
    } else if (is_leaf(root[vpn3])) {
        /* Demote 512G leaf -> 512 x 1G identity leaves. */
        uint64_t base = (root[vpn3] >> 10) << 12;
        uint64_t leaf_flags = root[vpn3] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                            PTE_G | PTE_A | PTE_D);

        l2 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l2[i] = pte_leaf(base + (uint64_t)i * GIGA_SIZE, leaf_flags);
        root[vpn3] = pte_next(l2);
    } else {
        l2 = (uint64_t *)((root[vpn3] >> 10) << 12);
    }

    /* Level 2 -> Level 1 */
    if (!(l2[vpn2] & PTE_V)) {
        l1 = page_alloc_table();
        l2[vpn2] = pte_next(l1);
    } else if (is_leaf(l2[vpn2])) {
        /* Demote 1G leaf -> 512 x 2M identity leaves. */
        uint64_t base = (l2[vpn2] >> 10) << 12;
        uint64_t leaf_flags = l2[vpn2] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                          PTE_G | PTE_A | PTE_D);

        l1 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l1[i] = pte_leaf(base + (uint64_t)i * MEGA_SIZE, leaf_flags);
        l2[vpn2] = pte_next(l1);
    } else {
        l1 = (uint64_t *)((l2[vpn2] >> 10) << 12);
    }

    /* Level 1 -> Level 0 */
    if (!(l1[vpn1] & PTE_V)) {
        l0 = page_alloc_table();
        l1[vpn1] = pte_next(l0);
    } else if (is_leaf(l1[vpn1])) {
        /* Demote 2M leaf -> 512 x 4K identity leaves. */
        uint64_t base = (l1[vpn1] >> 10) << 12;
        uint64_t leaf_flags = l1[vpn1] & (PTE_R | PTE_W | PTE_X | PTE_U |
                                          PTE_G | PTE_A | PTE_D);

        l0 = page_alloc_table();
        for (i = 0; i < 512; i++)
            l0[i] = pte_leaf(base + (uint64_t)i * PAGE_SIZE, leaf_flags);
        l1[vpn1] = pte_next(l0);
    } else {
        l0 = (uint64_t *)((l1[vpn1] >> 10) << 12);
    }

    l0[vpn0] = pte_leaf(pa, flags | PTE_A | PTE_D);
}

void page_enable_sv48(uint64_t *root) {
    uint64_t ppn = ((uint64_t)(uintptr_t)root) >> 12;
    uint64_t satp = (SATP_MODE_SV48 << SATP_MODE_SHIFT) | ppn;

    csr_write(satp, satp);
    sfence_vma_all();
}

/* ============================================================================
 * Generic Operations
 * ========================================================================= */

void page_disable(void) {
    csr_write(satp, 0);
    sfence_vma_all();
}

pt_mode_t page_get_mode(void) {
    uint64_t satp = csr_read(satp);
    return (pt_mode_t)((satp >> SATP_MODE_SHIFT) & 0xF);
}

void page_switch_mode(pt_mode_t new_mode, uint64_t *root) {
    if (root == 0 && new_mode != PT_MODE_BARE) {
        test_fail("page_switch_mode: root cannot be NULL for paging modes");
    }

    switch (new_mode) {
        case PT_MODE_BARE:
            page_disable();
            break;

        case PT_MODE_SV39:
            page_enable_sv39(root);
            break;

        case PT_MODE_SV48:
            page_enable_sv48(root);
            break;

        default:
            test_fail("page_switch_mode: unsupported mode");
    }
}
