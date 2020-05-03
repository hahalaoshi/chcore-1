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

// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <common/printk.h>
#include <common/types.h>

static inline __attribute__((always_inline)) u64 
read_fp() {
    u64 fp;
    __asm __volatile("mov %0, x29" : "=r" (fp));
    return fp;
}

__attribute__((optimize("O1")))
int mon_backtrace() {
    // fp: current frame pointer
    u64 *fp = (u64 *)read_fp();
    printk("Stack backtrace:\n");
    // x29 has been assigned to sp, so to get the read fp
    // fp != 0 continue tracing
    while (fp) {
        // l_fp: last frame pointer, l_lr: last return address
        u64 l_fp, l_lr;
        l_fp = *fp;
        l_lr = *((u64 *)l_fp + 1);
        u64 args[5] = {*(fp + 2), *(fp + 3), *(fp + 4), *(fp + 5), *(fp + 6)};
        printk("  LR %lx  FP %lx  Args %lx %lx %lx %lx %lx\n", l_lr, l_fp,
                args[0], args[1], args[2], args[3], args[4]);
        fp = (u64 *)l_fp;
    }

	return 0;
}
