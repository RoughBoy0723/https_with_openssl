#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int copyfile(char *argv,char *argv2)
{
int size,fd,fd2=0;
char buf[13];
if(fd=open(argv,O_RDONLY)<0)
{
return -1;
}
if(fd2=open(argv2,O_WRONLY|O_CREAT|O_TRUNC,0666)<0)
{
return -1;
close(fd);
}
while(size=read(fd,buf,13)>0)
{
write(fd2,buf,size);
}
close(fd);
close(fd2);
}
int main(int *argc,char *argv[])
{
copyfile(argv[1],argv[2]);
}
