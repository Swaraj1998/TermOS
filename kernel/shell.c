#include "shell.h"
#include "../libc/string.h"
#include "../libc/file.h"
#include "../libc/util.h"
#include "../cpu/ports.h"
#include "../cpu/timer.h"
#include "../libc/stdio.h"
#include "../fs/sfs.h"
#include "../programs/program.h"
#include "../sched/sched.h"

#define NB_COMMANDS 18
static char *cmd_all[] = {
    "help", "poweroff", "clear", "getpage", "freepage",
    "fread<>", "fwrite<>", "gettick", "test-stdin",
    "debugfs", "formatfs", "mountfs", "fcreate", "fremove<>",
    "fstat<>", "schedule", "taskview", "fexec<>"
};

void shell_parse_commands(char *str, int *cnt, char **cmd_list)
{
    char *ptr = str;
    char *start;
    int cmd_cnt = 0;

    while (*ptr != '\0' && cmd_cnt < MAX_TASKS) {
        while (*ptr == ' ') ptr++;
        start = ptr;
        while (*ptr != ';' && *ptr != '\0') ptr++;
        int len = ptr - start;

        char *cmd = (char*) kmalloc(len + 1);
        mem_cpy(start, cmd, len);
        cmd[len] = '\0';

        cmd_list[cmd_cnt++] = cmd;
        if (*ptr == ';') ptr++;
    }

    *cnt = cmd_cnt;
}

void shell_free_commands(int cnt, char **cmd_list)
{
    for (int i = 0; i < cnt; i++)
        free(str_len(cmd_list[i]) + 1);
}

void shell_run()
{
    char cmd[512];
    for (;;) {
        gets(cmd);
        shell_exec(cmd);
    }
}

void shell_exec(char *cmd)
{
    if (!str_cmp(cmd, "poweroff")) {
        kprint("[+] Shutting down...\n");
        delay(600);
        port_w16(0x604, 0x2000);    // qemu s/w shutdown

    } else if (!str_cmp(cmd, "clear")) {
        clear_screen();
        kprint("TermOS v1.0\n");

    } else if (!str_cmp(cmd, "getpage")) {
        char page_addr[16] = "";
        uint32_t page = kmalloc(10);
        hex_to_ascii(page, page_addr);
        kprint("\nPage address: ");
        kprint(page_addr);
        kprint("\n");

    } else if (!str_cmp(cmd, "freepage")) {
        free(10);

    } else if (!str_cmp_n(cmd, "fread ", 6)) {
        char *buf = (char*) kmallocz(512);
        int fd = ascii_to_int(cmd+6);
        if (sfs_read(fd, buf, 512, 0) < 0)
            kprint("ERR: Cannot read file");
        else
            kprint(buf);
        free(512);

    } else if (!str_cmp_n(cmd, "fwrite ", 7)) {
        char *buf = (char*) kmallocz(512);
        int fd, n;
        cmd += 7;

        fd = ascii_to_int(cmd);
        while (*cmd != ' ') cmd++;
        cmd++;

        n = str_len(cmd);
        mem_cpy(cmd, buf, n);
        if (sfs_write(fd, buf, n, 0) < 0)
            kprint("ERR: Cannot write to file");
        free(512);

    } else if (!str_cmp(cmd, "gettick")) {
        kprintd(timer_get_ticks());

    } else if (!str_cmp(cmd, "test-stdin")) {
        char str[64];
        do {
            gets(str);
            kprint(str);
            kprint("\n");
        } while (str[0] != '0');

    } else if (!str_cmp(cmd, "debugfs")) {
        sfs_debug();

    } else if (!str_cmp(cmd, "formatfs")) {
        sfs_format();

    } else if (!str_cmp(cmd, "mountfs")) {
        if (sfs_mount())
            kprint("\nERR: Cannot mount filesystem");

    } else if (!str_cmp(cmd, "fcreate")) {
        int ret = sfs_create();
        if(ret < 0) {
            kprint("\nERR: Cannot create file");
        } else {
            kprint("File created with fd: ");
            kprintd(ret);
        }

    } else if (!str_cmp_n(cmd, "fremove ", 8)) {
        cmd += 8;
        int fd = ascii_to_int(cmd);
        if (sfs_remove(fd) < 0)
            kprint("\nERR: Cannot remove file");

    } else if (!str_cmp_n(cmd, "fstat ", 6)) {
        cmd += 6;
        int fd = ascii_to_int(cmd);
        int ret = sfs_stat(fd);
        if (ret < 0) {
            kprint("\nERR: File does not exist");
        } else {
            kprint("\nFile size: ");
            kprintd(ret);
        }

    } else if (!str_cmp(cmd, "schedule")) {
        exec_task_queue();

    } else if (!str_cmp(cmd, "taskview")) {
        toggle_task_view();

    } else if (!str_cmp_n(cmd, "fexec ", 6)) {
        cmd += 6;
        int fd = ascii_to_int(cmd);
        if (sfs_stat(fd) < 0) {
            kprint("\nERR: File does not exist");
        } else {
            int cnt;
            char *cmd_list[MAX_TASKS];

            char buf[2048];
            sfs_read(fd, buf, MIN(sfs_stat(fd), 2048), 0);

            shell_parse_commands(buf, &cnt, cmd_list);
            for (int i = 0; i < cnt; i++)
                sched_new_task(cmd_list[i], 0);

            shell_free_commands(cnt, cmd_list);
            exec_task_queue();
        }
    } else if (!str_cmp(cmd, "help")) {
        kprint("System Commands:\n");
        int i;
        for (i = 0; i < NB_COMMANDS-1; i++) {
            kprint(cmd_all[i]);
            kprint(", ");
            if (i && i % 7 == 0)
                kprint("\n");
        }
        kprint(cmd_all[i]);
        kprint("\n\nAvailable programs:\n");
        print_program_names();

    } else {
        // schedule and execute immediately
        sched_new_task(cmd, 1);
    }
    kprint("\n>");
}
