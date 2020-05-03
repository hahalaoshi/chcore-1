//
// Created by blasi on 03/05/2020.
//
#include <common/kprint.h>
#include <common/macro.h>
#include <common/util.h>
#include <ipc/ipc.h>
#include <process/thread.h>



// return 0 means send fails, the receiver is not blocking
// return 1 means the receiver will get the message
u32 sys_ipc_send(u32 conn_cap, u64 msg) {
    struct ipc_connection *conn = NULL;
    BUG_ON(current_thread == NULL);
    conn = obj_get(current_thread->process, conn_cap, TYPE_CONNECTION);
    u64 *shared_adr = (u64 *)conn->buf.client_user_addr;
    // check whether it's ready
    if (*shared_adr == 0) {
        return 0;
    }
    // set message
    *(shared_adr + 1) = msg;
    // set not ready
    *shared_adr = 0;
    // success
    return 1;
}
