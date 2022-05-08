#include <iostream>
//#define _POSIX_C_SOURCE 200112L
//不知道为什么提示重复定义了
/* C standard library */
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* POSIX */
#include <unistd.h>
#include <sys/user.h>
#include <sys/wait.h>

/* Linux */
#include <syscall.h>
#include <sys/ptrace.h>

using namespace std;

//使用实例程序中的报错函数 实现看不是很懂 但是确实好用
//...与va args似乎表示可变参数 所以传递的时候会是那种形式
#define FATAL(...)                               \
    do                                           \
    {                                            \
        fprintf(stderr, "strace: " __VA_ARGS__); \
        fputc('\n', stderr);                     \
        exit(EXIT_FAILURE);                      \
    } while (0)

int main(int argc, char **argv)
{
    if (argc <= 1)
        FATAL("too few arguments: %d", argc);
        
    pid_t pid = fork();
    switch (pid)
    {
    case -1: /* error */
        FATAL("%s", strerror(errno));
    case 0: /* child */
        // traceme ：pid, addr, and data are ignored.
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        // execvp会替换接下来
        execvp(argv[1], argv + 1);
        FATAL("%s", strerror(errno));
    }

    //子进程在执行到 PTRACE_TRACEME 后会停住等待父进程指令。 在父进程中，使用：
    waitpid(pid, 0, 0);
    //这里 PTRACE_O_EXITKILL 指示子进程在父进程退出后立即退出
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    while(true)
    {
        /* Enter next system call */
        // syscall模式是监控的进程一直运行到下一个syscall的入口为止
        //然后父进程使用waitpid恢复子进程
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1)
            FATAL("%s", strerror(errno));
        if (waitpid(pid, 0, 0) == -1)
            FATAL("%s", strerror(errno));

        /* Gather system call arguments */
        /*读取子进程中进行 syscall 时各个 CPU 寄存器的值。
        由于是在 Linux 环境中，function call 遵循 Linux system call convention，
        参数按顺序排列在 rdi、rsi、rdx、r10、r8、r9 寄存器中，
        syscall number 置于 rax 寄存器中。
        通过读取这些寄存器，我们就可以知道参数的值和子进程具体要调用的 syscall。*/
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
            FATAL("%s", strerror(errno));
        long syscall = regs.orig_rax;

        /* Print a representation of the system call */
        fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)",
                syscall,
                (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
                (long)regs.r10, (long)regs.r8, (long)regs.r9);
        
        /* Run system call and stop on exit */
        //上次停在了入口 现在停在出口
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1)
            FATAL("%s", strerror(errno));
        if (waitpid(pid, 0, 0) == -1)
            FATAL("%s", strerror(errno));

        /* Get system call result */
        //在 syscall 出口处时，返回值存在 rax 寄存器中。 
        //我们可以通过读取寄存器 rax 来获取返回值，甚至是修改返回值：
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
        {
            fputs(" = ?\n", stderr);
            if (errno == ESRCH)
                exit(regs.rdi); // system call was _exit(2) or similar
            FATAL("%s", strerror(errno));
        }

        /* Print system call result */
        fprintf(stderr, " = %ld\n", (long)regs.rax);
    }
}