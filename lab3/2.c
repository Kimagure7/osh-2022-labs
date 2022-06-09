#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF 1024
#define MAX_USERS 32

struct USER
{
    //向所有用户发送信息 接受的时候也是全部接收 不需要使用fd_send了
    // fd_recv表示从哪里接受数据 定义与1.c中fd_send一样
    int fd_recv;
    int id;
};

int client[MAX_USERS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int used[MAX_USERS] = {0}; //用户池

void *handle_chat(void *data)
{
    struct USER *user = (struct USER *)data;
    char buffer[BUF * 2];
    char buffer_mess[BUF];
    ssize_t len;

    sprintf(buffer, "user %2d: ", user->id);

    while (1)
    {
        len = recv(user->fd_recv, buffer_mess, BUF - 24, 0);
        if (len <= 0)
        {
            // <0 : error   =0: 连接中断
            used[user->id] = 0;
            close(client[user->id]);
            return 0;
        }

        int temp = -1; //记录上一个\n位置
        // puts(buffer_mess);
        // fflush(stdout);
        for (int i = 0; i < len; i++)
        {
            // TODO：太长了而没有换行符
            if (buffer_mess[i] == '\n')
            {
                strncpy(buffer + 9, buffer_mess + temp +1, i - temp);
                // TODO:第二步处理 如果send没发全 就while一直发出去
                int length = i - temp + 9;
                int send_length = 0; //记录已发送的字节数
                int send_return = 0; // send的返回值
                pthread_mutex_lock(&mutex);
                for (int j = 0; j < MAX_USERS; j++)
                {
                    if (used[j] && j != user->id)
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
                pthread_mutex_unlock(&mutex);
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "user %2d: ", user->id);//恢复初始状态
                temp = i;                //+1 是为了跳过换行符
            }
        }
        if (temp == -1) //!= len也可以
        {
            //如果沒有換行符
            strncpy(buffer + 9, buffer_mess, strlen(buffer_mess)); //全弄过来
            int length = len + 9;
            int send_length = 0; //记录已发送的字节数
            int send_return = 0; // send的返回值
            pthread_mutex_lock(&mutex);
            for (int j = 0; j < MAX_USERS; j++)
            {
                if (used[j] && j != user->id)
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
            pthread_mutex_unlock(&mutex);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "user %2d: ", user->id); //恢复初始状态
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

    pthread_t thread[MAX_USERS];
    struct USER user[MAX_USERS];

    while (1)
    {
        int i;
        int temp_client = accept(fd, NULL, NULL);
        if (temp_client == -1)
        {
            perror("accept");
            return 1;
        }

        for (i = 0; i < MAX_USERS; i++)
        {
            if (used[i] == 0)
            { //找一个没使用的id分配
                used[i] = 1;
                client[i] = temp_client;
                user[i].fd_recv = temp_client;
                user[i].id = i;
                pthread_create(&thread[i], NULL, handle_chat, (void *)&user[i]);
                break;
            }
        }
        if (i == MAX_USERS)
        {
            perror("MAX_USERS");
            return 1;
        }
    }
    return 0;
}