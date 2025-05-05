#include "user.h"
int main()
{
    char buf[64];
    fd_t fd = open("/text", O_RDWR, 0);
    read(fd, buf, 64);
    buf[10] = 'H';
    buf[11] = 'E';
    buf[12] = 'L';
    buf[13] = 'L';
    buf[14] = 'O';
    write(fd, buf, 64);

    char *addr = (char *)mmap(NULL, 4096, VM_PROT_READ | VM_PROT_WRITE, 0, fd, 0);
    addr[0] = 'W';
    addr[1] = 'O';
    addr[2] = 'R';
    addr[3] = 'L';
    addr[4] = 'D';
    addr[5] = '!';

    int pid = fork();
    if (pid > 0) {
        while (1) {
            sleep(500);
        }
    }
    else {
        while (1) {
            sleep(200);
        }
    }
}
