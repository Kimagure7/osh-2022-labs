#include <iostream>
#define _POSIX_C_SOURCE 200112L
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

int main()
{
    pid_t pid = fork();
}