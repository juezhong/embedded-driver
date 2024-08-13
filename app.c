#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

char *write_buf = "Test application 测试程序。\n";
char read_buf[128];

int main(void)
{
    printf("%s", write_buf);

    // open file
    int fd = open("/dev/mem_ctl", O_RDWR);
    // filename use `mknod /dev/mem_ctl c xxx 0` generate
    // xxx via `cat /proc/devices` check

    // write data
    write(fd, write_buf, strlen(write_buf));
    close(fd);

    // reopen
    fd = open("/dev/mem_ctl", O_RDWR);
    // read data
    read(fd, read_buf, 128);
    // print
    printf("Read content: %s", read_buf);
    close(fd);
    return 0;
}