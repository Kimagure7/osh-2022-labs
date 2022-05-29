#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>

#define BUF 1024
#define MAX_USERS 32

int used[MAX_USERS] = {0};
int client[MAX_USERS] = {0};

void handle_chat(int id)
{
    char buffer[BUF * 2];
    char buffer_mess[BUF];
    ssize_t len;
    int first_recv = 1; //必须使用while 1 但是只能处理一次msg
    sprintf(buffer, "user %2d: ", id);

    while (1)
    {
        len = recv(client[id], buffer_mess, BUF - 24, 0);//需要注意这里已经是非阻塞的了
        if (len <= 0)
        {
            if (first_recv)
            {
                // <0 : error   =0: 连接中断
                used[id] = 0;
                close(client[id]);//这里似乎很关键 推测多次close会出现问题
                return;
            }
            return;
        }
        first_recv = 0;

        int temp = 0; //记录上一个\n位置
        // puts(buffer_mess);
        // fflush(stdout);
        for (int i = 0; i < len; i++)
        {
            // TODO：太长了而没有换行符
            if (buffer_mess[i] == '\n')
            {
                strncpy(buffer + 9, buffer_mess + temp, i + 1 - temp);
                // TODO:第二步处理 如果send没发全 就while一直发出去
                int length = strlen(buffer);
                int send_length = 0; //记录已发送的字节数
                int send_return = 0; // send的返回值
                for (int j = 0; j < MAX_USERS; j++)
                {
                    if (used[j] && j != id)
                    {
                        while (send_length < length)
                        {
                            send_return = send(client[j], buffer + send_length, length - send_length, 0); //每次都尝试发送剩下的所有信息
                            if (send_return == -1)
                            {
                                perror("send");
                                exit(-1);
                            }
                            send_length += send_return;
                        }
                        send_return = 0;
                        send_length = 0;
                    }
                }
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "user %2d: ", id); //恢复初始状态
                temp = i + 1;                      //+1 是为了跳过换行符
            }
        }
        if (temp != strlen(buffer_mess)) //!= len也可以
        {
            //如果沒有換行符
            strncpy(buffer + 9, buffer_mess, strlen(buffer_mess)); //全弄过来
            int length = strlen(buffer);
            int send_length = 0; //记录已发送的字节数
            int send_return = 0; // send的返回值
            for (int j = 0; j < MAX_USERS; j++)
            {
                if (used[j] && j != id)
                {
                    while (send_length < length)
                    {
                        send_return = send(client[j], buffer + send_length, length - send_length, 0); //每次都尝试发送剩下的所有信息
                        if (send_return == -1)
                        {
                            perror("send");
                            exit(-1);
                        }
                        send_length += send_return;
                    }
                    send_return = 0;
                    send_length = 0;
                }
            }
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "user %2d: ", id); //恢复初始状态
        }
        memset(buffer_mess, 0, sizeof(buffer_mess));
    }
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
    if (listen(fd, MAX_USERS))
    {
        perror("listen");
        return 1;
    }

    fd_set clients;
    int max_fd = fd;
    int i;
    while (1)
    {
        FD_ZERO(&clients);
        FD_SET(fd, &clients); //因为要侦听fd
        for (i = 0; i < MAX_USERS; i++)
        {
            if (used[i])
                FD_SET(client[i], &clients);
        }
        if (select(max_fd + 1, &clients, NULL, NULL, NULL) > 0)
        {
            if (FD_ISSET(fd, &clients))
            {
                //有新建连接
                int new_client = accept(fd, NULL, NULL);
                if (new_client == -1)
                {
                    perror("accept");
                    return 1;
                }
                fcntl(new_client, F_SETFL, fcntl(new_client, F_GETFL, 0) | O_NONBLOCK);

                if (max_fd < new_client)
                    max_fd = new_client; //更新套接字最大值

                for (i = 0; i < MAX_USERS; i++)
                {
                    if (!used[i])
                    {
                        used[i] = 1;
                        client[i] = new_client;
                        break;
                    }
                }
            }
            else
            {
                for (i = 0; i < MAX_USERS; i++)
                {
                    if (used[i] && FD_ISSET(client[i], &clients))
                    {
                        handle_chat(i);
                    }
                }
            }
        }
    }
    return 0;
}
