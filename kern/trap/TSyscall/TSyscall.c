#include <lib/debug.h>
#include <lib/pmap.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/syscall.h>

#include "import.h"

static char sys_buf[NUM_IDS][PAGESIZE];

/**
 * Copies a string from user into buffer and prints it to the screen.
 * This is called by the user level "printf" library as a system call.
 */
void sys_puts(void)
{
    
    unsigned int cur_pid;
    unsigned int str_uva, str_len;
    unsigned int remain, cur_pos, nbytes;

    cur_pid = get_curid();
    str_uva = syscall_get_arg2();
    str_len = syscall_get_arg3();

    if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI)) {
        syscall_set_errno(E_INVAL_ADDR);
        return;
    }

    remain = str_len;
    cur_pos = str_uva;

    while (remain) {
        if (remain < PAGESIZE - 1)
            nbytes = remain;
        else
            nbytes = PAGESIZE - 1;

        if (pt_copyin(cur_pid, cur_pos, sys_buf[cur_pid], nbytes) != nbytes) {
            syscall_set_errno(E_MEM);
            return;
        }

        sys_buf[cur_pid][nbytes] = '\0';
        KERN_INFO("%s", sys_buf[cur_pid]);

        remain -= nbytes;
        cur_pos += nbytes;
    }

    syscall_set_errno(E_SUCC);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];
extern uint8_t _binary___obj_user_fork_test_fork_test_start[];

/**
 * Spawns a new child process.
 * The user level library function sys_spawn (defined in user/include/syscall.h)
 * takes two arguments [elf_id] and [quota], and returns the new child process id
 * or NUM_IDS (as failure), with appropriate error number.
 * Currently, we have three user processes defined in user/pingpong/ directory,
 * ping, pong, and ding.
 * The linker ELF addresses for those compiled binaries are defined above.
 * Since we do not yet have a file system implemented in mCertiKOS,
 * we statically load the ELF binaries into the memory based on the
 * first parameter [elf_id].
 * For example, ping, pong, ding and fork_test correspond to the elf_ids
 * 1, 2, 3, and 4, respectively.
 * If the parameter [elf_id] is none of these, then it should return
 * NUM_IDS with the error number E_INVAL_PID. The same error case apply
 * when the proc_create fails.
 * Otherwise, you should mark it as successful, and return the new child process id.
 */
void sys_spawn(void)
{
    unsigned int elf_id = syscall_get_arg2();
    unsigned int quota = syscall_get_arg3();
    void *elf_addr;
    unsigned int child;

    switch (elf_id) {
    case 1:
        elf_addr = _binary___obj_user_pingpong_ping_start;
        break;
    case 2:
        elf_addr = _binary___obj_user_pingpong_pong_start;
        break;
    case 3:
        elf_addr = _binary___obj_user_pingpong_ding_start;
        break;
    case 4:
        elf_addr = _binary___obj_user_fork_test_fork_test_start;
        break;
    default:
        syscall_set_errno(E_INVAL_PID);
        syscall_set_retval1(NUM_IDS);
        return;
    }

    child = proc_create(elf_addr, quota);
    if (child != NUM_IDS) {
        syscall_set_errno(E_SUCC);
    } else {
        syscall_set_errno(E_INVAL_PID);
    }
    syscall_set_retval1(child);
}

/**
 * Yields to another thread/process.
 * The user level library function sys_yield (defined in user/include/syscall.h)
 * does not take any argument and does not have any return values.
 * Do not forget to set the error number as E_SUCC.
 */
void sys_yield(void)
{
    thread_yield();
    syscall_set_errno(E_SUCC);
}

// Your implementation of fork
void sys_fork()
{
    unsigned int child = proc_fork();
    if (child != NUM_IDS) {
        syscall_set_errno(E_SUCC);
    } else {
        syscall_set_errno(E_INVAL_PID);
    }
    syscall_set_retval1(child);
}
