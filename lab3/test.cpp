#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <iostream>
using namespace std;
int main()
{
    char buffer[1024] = "Message: ";
    char buffer_mess[1024] = "hi\ni'm hello Kitty\nand you?\n";
    int len = strlen(buffer_mess);
    int temp = 0; //记录上一个\n位置
    for (int i = 0; i < len; i++)
    {
        if (buffer_mess[i] == '\n')
        {
            strncpy(buffer + 9, buffer_mess + temp, i + 1 - temp);
            printf("%s", buffer);
            memset(buffer,0,sizeof(buffer));
            strcpy(buffer, "Message: "); //恢复初始状态
            temp = i + 1; //+1 是为了跳过换行符
        }
    }
}