#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct Pipe
{
    int fd_send;
    int fd_recv;
};
void *handle_chat(void *data)
{
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[1024] = "Message: ";
    char buffer_mess[1024] = ""; //这个用来接受纯净的信息 便于分离
    ssize_t len;
    //读进来自带换行符
    while ((len = recv(pipe->fd_send, buffer_mess, 1000, 0)) > 0)
    {
        int temp = 0; //记录上一个\n位置
        //puts(buffer_mess);
        //fflush(stdout);
        for (int i = 0; i < len; i++)
        {
            // TODO：太长了而没有换行符
            if (buffer_mess[i] == '\n')
            {
                strncpy(buffer + 9, buffer_mess + temp, i + 1 - temp);
                // TODO:第二步处理 如果send没发全 就while一直发出去
                int length = strlen(buffer);
                int send_length = 0;         //记录已发送的字节数
                while (send_length < length) //理论上只要有换行符就不会触发这里
                {
                    send_length += send(pipe->fd_recv, buffer + send_length, length - send_length, 0); //每次都尝试发送剩下的所有信息
                }
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Message: "); //恢复初始状态
                temp = i + 1;                //+1 是为了跳过换行符
            }
        }
        if (temp != strlen(buffer_mess))
        {
            //如果沒有換行符
            strncpy(buffer + 9, buffer_mess, strlen(buffer_mess));//全弄过来
            int length = strlen(buffer);
            int send_length = 0;         //记录已发送的字节数
            while (send_length < length) //理论上只要有换行符就不会触发这里
            {
                send_length += send(pipe->fd_recv, buffer + send_length, length - send_length, 0); //每次都尝试发送剩下的所有信息
            }
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "Message: "); //恢复初始状态
        }
        memset(buffer_mess, 0, sizeof(buffer_mess));
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2))
    {
        perror("listen");
        return 1;
    }
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1)
    {
        perror("accept");
        return 1;
    }
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
