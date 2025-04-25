// #include "user.h"
// int main()
// {
//     pid_t pid;
//     while (1) {
//         pid = fork();
//         if (pid > 0)
//             waitpid(-1, NULL, 0);
//         else if (pid == 0)
//             exec("/bin/sh", NULL);
//         else
//             debug(0);
//     }
// }
#include "user.h"


// ctrl + l ->12
static int a = 0;
void my_add(int)
{
    a++;
}


int main()
{
    sigaction(12, (__sighandler_t)my_add);
    while (1) {
        sleep(300);
        debug(a);
    }
}
