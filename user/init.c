#include "user.h"
int main()
{
    pid_t pid;
    while (1) {
        pid = fork();
        if (pid > 0)
            waitpid(-1, NULL, 0);
        else if (pid == 0)
            exec("/bin/sh", NULL);
        else
            debug(0);
    }
}
