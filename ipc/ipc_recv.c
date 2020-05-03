//
// Created by blasi on 03/05/2020.
//
#include <common/util.h>
#include <ipc/ipc.h>
#include <process/thread.h>
#include <kernel/common/lock.h>

u64 sys_ipc_recv() {
    // Allow this syscall to block
    unlock_kernel();
    while (current_thread->active_conn == NULL) {}
    // avoid the assembler opt
    volatile u64 *shared_adr = (u64 *)current_thread->active_conn->buf.server_user_addr;
    // set ready = 1
    *shared_adr = 1;
    while (*shared_adr == 1) {
        // still ready, block here
        // let the sender set ready = 0
    }
    // reacquire the lock, which will be released after syscall
    lock_kernel();
    return *(shared_adr + 1);
}
