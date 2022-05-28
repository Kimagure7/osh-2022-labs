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
    buffer[6]=0;
    printf("%ld\n",strlen(buffer));
}