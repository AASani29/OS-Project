// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <lib/debug.h>
#include <lib/elf.h>
#include <lib/types.h>
#include <lib/gcc.h>
#include <lib/string.h>
#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/monitor.h>
#include <dev/console.h>
#include <vmm/MPTIntro/export.h>
#include <vmm/MPTNew/export.h>
#include <kern/ipc/pubsub.h> // Include the Pub/Sub IPC header
#include <kern/ipc/publisher.c>
#include <kern/ipc/subscriber.c>

#define CMDBUF_SIZE 80  // enough for one VGA text line

struct Command {
    const char *name;
    const char *desc;
    // return -1 to force monitor to exit
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

// Subscriber callback for testing
void pubsub_callback(struct Message *msg) {
    dprintf("Received message: %s\n", msg->data); // Replace cprintf with dprintf
}
// Pub/Sub command
int mon_pubsub(int argc, char **argv, struct Trapframe *tf) {
    if (argc < 2) {
        dprintf("Usage: pubsub <publish|subscribe> <topic> [message]\n");
        return 0;
    }

    if (strcmp(argv[1], "publish") == 0) {
        if (argc < 4) {
            dprintf("Usage: pubsub publish <topic> <message>\n");
            return 0;
        }
        publish(argv[2], argv[3]);
        dprintf("Published message to topic '%s': %s\n", argv[2], argv[3]);
    } else if (strcmp(argv[1], "subscribe") == 0) {
        if (argc < 3) {
            dprintf("Usage: pubsub subscribe <topic>\n");
            return 0;
        }
        subscribe(argv[2], pubsub_callback);
        dprintf("Subscribed to topic '%s'\n", argv[2]);
    } else {
        dprintf("Unknown command: %s\n", argv[1]);
    }

    return 0;
}


// Test function for Pub/Sub
int mon_test_pubsub(int argc, char **argv, struct Trapframe *tf) {
    // Subscribe to a test topic
    subscribe("test_topic", pubsub_callback);
    dprintf("Subscribed to 'test_topic'\n");

    // Publish a message to the test topic
    publish("test_topic", "Hello, this is a test message!");
    dprintf("Published message to 'test_topic'\n");

    return 0;
}
// kern/lib/monitor.c
int mon_list_topics(int argc, char **argv, struct Trapframe *tf) {
    list_topics();  // Call the new function
    return 0;
}
// kern/lib/monitor.c
int mon_show_queue(int argc, char **argv, struct Trapframe *tf) {
    if (argc < 2) {
        dprintf("Usage: show_queue <topic_name>\n");
        return 0;
    }
    show_queue(argv[1]);
    return 0;
}

// Add the test command to the commands array
static struct Command commands[] = {
    {"help", "Display this list of commands", mon_help},
    {"kerninfo", "Display information about the kernel", mon_kerninfo},
    {"startuser", "Start the user idle process", mon_start_user},
    {"pubsub", "Publish/Subscribe IPC commands", mon_pubsub},
    {"test_pubsub", "Test the Pub/Sub IPC system", mon_test_pubsub}, // Add this line
    {"list_topics", "Show active Pub/Sub topics", mon_list_topics}, // New entry
    {"show_queue", "Show messages in a topic's queue", mon_show_queue},
};

#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

extern uint8_t _binary___obj_user_idle_idle_start[];
extern unsigned int proc_create(void *elf_addr, unsigned int quota);
extern void tqueue_remove(unsigned int chid, unsigned int pid);
extern void tcb_set_state(unsigned int pid, unsigned int state);
extern void set_curid(unsigned int curid);
extern void kctx_switch(unsigned int from_pid, unsigned int to_pid);

/***** Implementations of basic kernel monitor commands *****/
int mon_help(int argc, char **argv, struct Trapframe *tf)
{
    int i;

    for (i = 0; i < NCOMMANDS; i++)
        dprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
    extern uint8_t start[], etext[], edata[], end[];

    dprintf("Special kernel symbols:\n");
    dprintf("  start  %08x\n", start);
    dprintf("  etext  %08x\n", etext);
    dprintf("  edata  %08x\n", edata);
    dprintf("  end    %08x\n", end);
    dprintf("Kernel executable memory footprint: %dKB\n",
            ROUNDUP(end - start, 1024) / 1024);
    return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // TODO
    return 0;
}

int mon_start_user(int argc, char **argv, struct Trapframe *tf)
{
    unsigned int idle_pid;
    idle_pid = proc_create(_binary___obj_user_idle_idle_start, 10000);
    KERN_DEBUG("process idle %d is created.\n", idle_pid);

    KERN_INFO("Start user-space ... \n");

    tqueue_remove(NUM_IDS, idle_pid);
    tcb_set_state(idle_pid, TSTATE_RUN);
    set_curid(idle_pid);
    kctx_switch(0, idle_pid);

    KERN_PANIC("mon_start_user() should never reach here.\n");
    return 0;
}

/***** Kernel monitor command interpreter *****/
#define WHITESPACE "\t\r\n "
#define MAXARGS    16

static int runcmd(char *buf, struct Trapframe *tf)
{
    int argc;
    char *argv[MAXARGS];
    int i;

    // Parse the command buffer into whitespace-separated arguments
    argc = 0;
    argv[argc] = 0;
    while (1) {
        // gobble whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // save and scan past next arg
        if (argc == MAXARGS - 1) {
            dprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    // Lookup and invoke the command
    if (argc == 0)
        return 0;
    for (i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }
    dprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void monitor(struct Trapframe *tf)
{
    char *buf;

    dprintf("\n****************************************\n\n");
    dprintf("Welcome to the mCertiKOS kernel monitor!\n");
    dprintf("\n****************************************\n\n");
    dprintf("Type 'help' for a list of commands.\n");

    while (1) {
        buf = (char *) readline("$> ");
        if (buf != NULL)
            if (runcmd(buf, tf) < 0)
                break;
    }
}