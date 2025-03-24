#include "user.h"
int main()
{
    pid_t pid;
    while (1) {
        pid = fork();
        if (pid == 0) {
            while (1)
                sleep(1000);
        }
        else if (pid > 0) {
            sleep(300);
        }
        else
            debug(0);
    }
}
