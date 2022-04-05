#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
int main(){
	char buf[20]= "error size";
	long return_value=0;
	return_value = syscall(548,buf,20);
	printf("return_value = %ld, buf = %s\n", return_value,buf);
	char buf2[20] = "error size";
	return_value = syscall(548,buf2,1);
	printf("return_value = %ld, buf = %s\n", return_value,buf2);
	while(1){}
}
