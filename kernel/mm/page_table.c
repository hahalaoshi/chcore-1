/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

#ifdef CHCORE

#include <common/util.h>
#include <common/kmalloc.h>

#endif

#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <common/printk.h>
#include <common/mm.h>
#include <common/mmu.h>
#include <common/kprint.h>
#include "page_table.h"

/* Page_table.c: Use simple impl for debugging now. */


/* pgd, pud, pmd, pte */
/* set ttbr0_el1 to paddr */
extern void set_ttbr0_el1(paddr_t);

/* flush all the tlb in every core*/
extern void flush_tlb(void);

/* set ttbr0 */
void set_page_table(paddr_t pgtbl) {
    /* XXX Perf: maybe check whether write ttbr0_el1 is needed */
    set_ttbr0_el1(pgtbl);
}

#define USER_PTE 0
#define KERNEL_PTE 1

/*
 * the 3rd arg means the kind of PTE.
 */
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind) {
    int isblock = !!(flags & L1_BLOCK) || !!(flags & L2_BLOCK);
    if (isblock) {
        /* l1 block and l2 block have similar structures */
        if (kind == KERNEL_PTE) {
            if (flags & VMR_WRITE)
                entry->l1_block.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_NA;
            else
                entry->l1_block.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_NA;
            entry->l1_block.UXN = ARM64_MMU_ATTR_PAGE_UXN;
            if (flags & VMR_EXEC)
                entry->l1_block.PXN = ARM64_MMU_ATTR_PAGE_PX;
            else
                entry->l1_block.PXN = ARM64_MMU_ATTR_PAGE_PXN;
        } else {
            if (flags & VMR_WRITE)
                entry->l1_block.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
            else
                entry->l1_block.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;
            entry->l1_block.PXN = ARM64_MMU_ATTR_PAGE_PXN;
            if (flags & VMR_EXEC)
                entry->l1_block.UXN = ARM64_MMU_ATTR_PAGE_UX;
            else
                entry->l1_block.UXN = ARM64_MMU_ATTR_PAGE_UXN;
        }
        entry->l1_block.AF = ARM64_MMU_ATTR_PAGE_AF_ACCESSED;
        // inner sharable
        entry->l1_block.SH = INNER_SHAREABLE;
        // memory type
        entry->l1_block.attr_index = NORMAL_MEMORY;
        entry->l1_block.is_table = 0;
        entry->l1_block.is_valid = 1;
    } else {
        if (kind == KERNEL_PTE) {
            if (flags & VMR_WRITE)
                entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_NA;
            else
                entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_NA;
            entry->l3_page.UXN = ARM64_MMU_ATTR_PAGE_UXN;
            if (flags & VMR_EXEC)
                entry->l3_page.PXN = ARM64_MMU_ATTR_PAGE_PX;
            else
                entry->l3_page.PXN = ARM64_MMU_ATTR_PAGE_PXN;
        } else {
            if (flags & VMR_WRITE)
                entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
            else
                entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;
            entry->l3_page.PXN = ARM64_MMU_ATTR_PAGE_PXN;
            if (flags & VMR_EXEC)
                entry->l3_page.UXN = ARM64_MMU_ATTR_PAGE_UX;
            else
                entry->l3_page.UXN = ARM64_MMU_ATTR_PAGE_UXN;
        }
        entry->l3_page.AF = ARM64_MMU_ATTR_PAGE_AF_ACCESSED;
        // not global
        //entry->l3_page.nG = 1;
        // inner sharable
        entry->l3_page.SH = INNER_SHAREABLE;
        // memory type
        entry->l3_page.attr_index = NORMAL_MEMORY;
        entry->l3_page.is_page = 1;
        entry->l3_page.is_valid = 1;
    }
    return 0;
}

/* entry: page table entry for both page and table, not block, 4kb page */
#define GET_PADDR_IN_PTE(entry) \
    (((u64)entry->table.next_table_addr) << PAGE_SHIFT)
/* virtual address of the next page table page*/
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1)

/*
 * Find next page table page for the "va".
 *
 * cur_ptp: current page table page
 * level:   current ptp level
 *
 * next_ptp: returns "next_ptp"
 * pte     : returns "pte" (points to next_ptp) in "cur_ptp"
 *
 * alloc: if true, allocate a ptp when missing
 *
 */
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va,
                        ptp_t **next_ptp, pte_t **pte, bool alloc) {
    u32 index = 0;
    pte_t *entry;

    if (cur_ptp == NULL)
        return -ENOMAPPING;

    /* 9 bits index */
    switch (level) {
        case 0:
            index = GET_L0_INDEX(va);
            break;
        case 1:
            index = GET_L1_INDEX(va);
            break;
        case 2:
            index = GET_L2_INDEX(va);
            break;
        case 3:
            index = GET_L3_INDEX(va);
            break;
        default:
            BUG_ON(1);
    }

    entry = &(cur_ptp->ent[index]);
    if (IS_PTE_INVALID(entry->pte)) {
        if (alloc == false) {
            *pte = entry;
            return -ENOMAPPING;
        } else {
            /* alloc a new page table page */
            ptp_t *new_ptp;
            paddr_t new_ptp_paddr;
            pte_t new_pte_val;

            /* alloc a single physical page as a new page table page */
            /* get an order 0 page from the buddy system, virtual address*/
            new_ptp = get_pages(0);
            BUG_ON(new_ptp == NULL);
            memset((void *) new_ptp, 0, PAGE_SIZE);
            new_ptp_paddr = virt_to_phys((vaddr_t) new_ptp);

            new_pte_val.pte = 0;
            new_pte_val.table.is_valid = 1;
            new_pte_val.table.is_table = 1;
            /* page frame number*/
            new_pte_val.table.next_table_addr
                    = new_ptp_paddr >> PAGE_SHIFT;

            /* same effect as: cur_ptp->ent[index] = new_pte_val; */
            entry->pte = new_pte_val.pte;
        }
    }

    *next_ptp = (ptp_t *) GET_NEXT_PTP(entry);
    *pte = entry;
    if (IS_PTE_TABLE(entry->pte))
        return NORMAL_PTP;
    else
        return BLOCK_PTP;
}


// TODO: You can invoke set_pte_flags(),get_next_ptp(), flush_tlb()
/*
 * Translate a va to pa, and get its pte for the flags
 */
/*
 * query_in_pgtbl: translate virtual address to physical 
 * address and return the corresponding page table entry
 * 
 * pgtbl @ ptr for the first level page table(pgd) virtual address
 * va @ query virtual address
 * pa @ return physical address
 * entry @ return page table entry
 * 
 * Hint: check the return value of get_next_ptp, if ret == BLOCK_PTP
 * return the pa and block entry immediately
 */
int query_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry) {
    ptp_t *cur_ptp = (ptp_t *) pgtbl, *next_ptp = NULL;
    int ret = get_next_ptp(cur_ptp, 0, va, &next_ptp, entry, false);
    if (ret == -ENOMAPPING) {
        return ret;
    }
    cur_ptp = (ptp_t *) next_ptp;
    ret = get_next_ptp(cur_ptp, 1, va, &next_ptp, entry, false);
    if (ret == BLOCK_PTP) {
        *pa =  ((*entry)->l1_block.pfn << (PAGE_SHIFT + ARM64_MMU_L1_BLOCK_ORDER)) | (va & ARM64_MMU_L1_BLOCK_MASK);
        return ret;
    }
    if (ret == -ENOMAPPING) {
        return ret;
    }
    cur_ptp = (ptp_t *) next_ptp;
    ret = get_next_ptp(cur_ptp, 2, va, &next_ptp, entry, false);
    if (ret == BLOCK_PTP) {
        *pa =  ((*entry)->l2_block.pfn << (PAGE_SHIFT + ARM64_MMU_L2_BLOCK_ORDER)) | (va & ARM64_MMU_L2_BLOCK_MASK);
        return ret;
    }
    if (ret == -ENOMAPPING) {
        return ret;
    }
    cur_ptp = (ptp_t *) next_ptp;
    ret = get_next_ptp(cur_ptp, 3, va, &next_ptp, entry, false);
    if (ret == -ENOMAPPING) {
        return ret;
    }
    *pa =  ((*entry)->l3_page.pfn << PAGE_SHIFT) | (va & 0xfff);
    return NORMAL_PTP;
}

/*
 * map_range_in_pgtbl: map the virtual address [va:va+size] to 
 * physical address[pa:pa+size] in given pgtbl
 *
 * pgtbl @ ptr for the first level page table(pgd) virtual address, *pgtbl is the virtual address
 * va @ start virtual address
 * pa @ start physical address
 * len @ mapping size
 * flags @ corresponding attribution bit
 *
 * Hint: In this function you should first invoke the get_next_ptp()
 * to get the each level page table entries. Read type pte_t carefully
 * and it is convenient for you to call set_pte_flags to set the page
 * permission bit. Don't forget to call flush_tlb at the end of this function 
 */
int map_range_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t pa,
                       size_t len, vmr_prop_t flags) {
    bool kernel, block1, block2;
    kernel = !!(flags & KERNEL_PT);
    block1 = !!(flags & L1_BLOCK);
    block2 = !!(flags & L2_BLOCK);
    u64 size;
    if (block1) {
        size = ARM64_MMU_L1_BLOCK_SIZE;
    } else if (block2) {
        size = ARM64_MMU_L2_BLOCK_SIZE;
    } else {
        size = ARM64_MMU_L3_PAGE_SIZE;
    }

    len = len + va % size;
    pa = pa - va % size;
    va = va - va % size;
    while (len > 0) {
        ptp_t *next_ptp = NULL, *cur_ptp = (ptp_t *) pgtbl;
        pte_t *entry = NULL;
        get_next_ptp(cur_ptp, 0, va, &next_ptp, &entry, true);
        BUG_ON(next_ptp == NULL);
        cur_ptp = (ptp_t *) next_ptp;
        get_next_ptp(cur_ptp, 1, va, &next_ptp, &entry, true);
        if (block1) {
            entry->pte = 0;
            set_pte_flags(entry, flags, kernel);
            entry->l1_block.pfn = pa >> (PAGE_SHIFT + ARM64_MMU_L1_BLOCK_ORDER);
            goto next_round;
        }
        BUG_ON(next_ptp == NULL);
        cur_ptp = (ptp_t *) next_ptp;
        get_next_ptp(cur_ptp, 2, va, &next_ptp, &entry, true);
        if (block2) {
            entry->pte = 0;
            set_pte_flags(entry, flags, kernel);
            entry->l2_block.pfn = pa >> (PAGE_SHIFT + ARM64_MMU_L2_BLOCK_ORDER);
            goto next_round;
        }
        BUG_ON(next_ptp == NULL);
        cur_ptp = (ptp_t *) next_ptp;
        get_next_ptp(cur_ptp, 3, va, &next_ptp, &entry, false);
        set_pte_flags(entry, flags, kernel);
        entry->l3_page.pfn = pa >> PAGE_SHIFT;
        entry->l3_page.is_valid = 1;
        entry->l3_page.is_page = 1;
        next_round:
        va = va + size;
        pa = pa + size;
        if (len > size) {
            len -= size;
        } else {
            break;
        }
    }
    flush_tlb();
    return 0;
}


/*
 * unmap_range_in_pgtble: unmap the virtual address [va:va+len]
 * 
 * pgtbl @ ptr for the first level page table(pgd) virtual address
 * va @ start virtual address
 * len @ unmapping size
 * 
 * Hint: invoke get_next_ptp to get each level page table, don't 
 * forget the corner case that the virtual address is not mapped.
 * call flush_tlb() at the end of function
 * 
 */
int unmap_range_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, size_t len) {
    len = len + va % PAGE_SIZE;
    va = va - va % PAGE_SIZE;
    while (len > 0) {
        ptp_t *next_ptp = NULL, *cur_ptp = (ptp_t *) pgtbl;
        pte_t *entry = NULL;
        int ret = get_next_ptp(cur_ptp, 0, va, &next_ptp, &entry, false);
        if (next_ptp == NULL || ret == -ENOMAPPING) {
            va += PAGE_SIZE;
            len -= PAGE_SIZE;
            continue;
        }
        cur_ptp = (ptp_t *) next_ptp;
        ret = get_next_ptp(cur_ptp, 1, va, &next_ptp, &entry, false);
        if (ret == BLOCK_PTP) {
            memset(entry, 0, sizeof(pte_t));
            va += ARM64_MMU_L1_BLOCK_SIZE;
            len -= ARM64_MMU_L1_BLOCK_SIZE;
            continue;
        }
        if (next_ptp == NULL || ret == -ENOMAPPING) {
            va += PAGE_SIZE;
            len -= PAGE_SIZE;
            continue;
        }
        cur_ptp = (ptp_t *) next_ptp;
        ret = get_next_ptp(cur_ptp, 2, va, &next_ptp, &entry, false);
        if (ret == BLOCK_PTP) {
            memset(entry, 0, sizeof(pte_t));
            len -= ARM64_MMU_L2_BLOCK_SIZE;
            va = va + ARM64_MMU_L2_BLOCK_SIZE;
            continue;
        }
        if (next_ptp == NULL || ret == -ENOMAPPING) {
            va = va + PAGE_SIZE;
            len -= PAGE_SIZE;
            continue;
        }
        cur_ptp = (ptp_t *) next_ptp;
        get_next_ptp(cur_ptp, 3, va, &next_ptp, &entry, false);
        if (ret == NORMAL_PTP && entry->l3_page.is_valid) {
            memset(entry, 0, sizeof(pte_t));
            va += PAGE_SIZE;
            len -= PAGE_SIZE;
            continue;
        }
    }
    flush_tlb();
    return 0;
}
