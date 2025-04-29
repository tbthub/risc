// #include "user.h"
// static char *str[] = {"hello", "world", NULL};
// int main()
// {
//     while (1) {
//         pid_t pid = fork();
//         if (pid == 0) {
//             exec("/bin/cat", str);
//         }
//         else if (pid > 0) {
//             waitpid(-1, NULL, 0);
//         }
//         else {
//             debug(0);
//         }
//     }
// }

#include "user.h"
int main()
{
   sleep(100);
   return 0;
}
