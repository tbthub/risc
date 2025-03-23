#include "user.h"
int main()
{
    pid_t pid;
    while (1) {
        pid = fork();
        if (pid == 0) {
            exec("/sh", NULL);
        }
        else if (pid > 0) {
            waitpid(-1, NULL, 0);
        }
        else {
            debug(100);
        }
    }
}
